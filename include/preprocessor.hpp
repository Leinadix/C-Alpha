#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>

namespace calpha {
    class Preprocessor {
    private:
        std::unordered_set<std::string> processedFiles;
        std::filesystem::path currentDir;

        std::string processImports(const std::string& source, const std::string& currentFile);
        std::string readFile(const std::string& filename);
        std::string generateNamespace(const std::string& code, const std::string& filename);
        std::string sanitizeIdentifier(const std::string& name);

    public:
        explicit Preprocessor(const std::string& workingDir = "");

        std::string process(const std::string& source, const std::string& mainFile);

        // Utility functions
        static bool isImportStatement(const std::string& line);
        static std::string extractFilename(const std::string& importLine);
    };
}