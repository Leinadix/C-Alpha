#ifndef PREPROCESSOR_HPP
#define PREPROCESSOR_HPP

#include <filesystem>
#include <string>
#include <unordered_set>

namespace calpha {
class Preprocessor {
  private:
    std::unordered_set<std::string> processedFiles;
    std::filesystem::path currentDir;

    std::string processImports(const std::string &source,
                               const std::string &currentFile);
    std::string readFile(const std::string &filename);
    std::string generateNamespace(const std::string &code,
                                  const std::string &filename);
    std::string sanitizeIdentifier(const std::string &name);

  public:
    explicit Preprocessor(const std::string &workingDir = "");

    std::string process(const std::string &source, const std::string &mainFile);

    // Utility functions
    static bool isImportStatement(const std::string &line);
    static std::string extractFilename(const std::string &importLine);
};
} // namespace calpha

#endif // PREPROCESSOR_HPP