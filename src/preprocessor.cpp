#include "preprocessor.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace calpha {

Preprocessor::Preprocessor(const std::string& workingDir)
    : currentDir(workingDir.empty() ? std::filesystem::current_path() : workingDir) {}

std::string Preprocessor::process(const std::string& source, const std::string& mainFile) {
    processedFiles.clear();
    return processImports(source, mainFile);
}

std::string Preprocessor::processImports(const std::string& source, const std::string& currentFile) {
    std::istringstream stream(source);
    std::ostringstream result;
    std::string line;
    
    // Get the directory of the current file
    std::filesystem::path currentFilePath = std::filesystem::absolute(currentFile);
    std::filesystem::path currentFileDir = currentFilePath.parent_path();
    
    // Add current file to processed set to prevent circular imports
    processedFiles.insert(currentFilePath.string());
    
    while (std::getline(stream, line)) {
        if (isImportStatement(line)) {
            std::string importFile = extractFilename(line);
            std::filesystem::path importPath = currentFileDir / importFile;
            std::string absolutePath = std::filesystem::absolute(importPath).string();
            
            // Check for circular imports
            if (processedFiles.find(absolutePath) != processedFiles.end()) {
                throw std::runtime_error("Circular import detected: " + importFile);
            }
            
            // Read and process the imported file
            std::string importedCode = readFile(importPath.string());
            importedCode = processImports(importedCode, absolutePath);
            
            // Wrap imported code in a namespace
            result << generateNamespace(importedCode, importFile) << "\n";
        } else {
            result << line << "\n";
        }
    }
    
    return result.str();
}

std::string Preprocessor::readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string Preprocessor::generateNamespace(const std::string& code, const std::string& filename) {
    std::string ns = sanitizeIdentifier(filename);
    
    // Convert filename to absolute path
    std::filesystem::path filePath = std::filesystem::absolute(currentDir / filename);
    std::string absolutePath = filePath.string();
    
    std::ostringstream result;
    result << "// Start of imported file: " << absolutePath << "\n";
    result << "layout __import_" << ns << " {\n";
    result << "    int _dummy;\n";
    result << "};\n\n";
    result << code << "\n";
    result << "// End of imported file: " << absolutePath << "\n";
    
    return result.str();
}

std::string Preprocessor::sanitizeIdentifier(const std::string& name) {
    std::string result;
    for (char c : name) {
        if (std::isalnum(c)) {
            result += c;
        } else {
            result += '_';
        }
    }
    return result;
}

bool Preprocessor::isImportStatement(const std::string& line) {
    std::istringstream iss(line);
    std::string word;
    iss >> word;
    return word == "import";
}

std::string Preprocessor::extractFilename(const std::string& importLine) {
    size_t start = importLine.find('"');
    if (start == std::string::npos) {
        throw std::runtime_error("Invalid import statement: missing opening quote");
    }
    
    size_t end = importLine.find('"', start + 1);
    if (end == std::string::npos) {
        throw std::runtime_error("Invalid import statement: missing closing quote");
    }
    
    return importLine.substr(start + 1, end - start - 1);
}

} // namespace calpha 