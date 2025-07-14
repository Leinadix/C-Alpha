# Contributing to Alpha-C Compiler

Thank you for your interest in contributing to the Alpha-C Compiler project! This document provides guidelines and information for contributors.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Submitting Changes](#submitting-changes)
- [Testing](#testing)
- [Documentation](#documentation)
- [Issue Reporting](#issue-reporting)
- [Pull Request Process](#pull-request-process)

## Code of Conduct

This project adheres to a Code of Conduct that we expect all contributors to follow. Please read and follow it in all your interactions with the project.

## Getting Started

### Prerequisites

- **C++ Compiler**: GCC 10+ or Clang 12+ with C++20 support
- **CMake**: Version 3.30 or higher
- **Git**: For version control
- **clang-format**: For code formatting
- **clang-tidy**: For static analysis
- **Doxygen**: For documentation generation

### Development Setup

1. **Fork the repository** on GitHub
2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/alpha-c.git
   cd alpha-c
   ```

3. **Set up the upstream remote**:
   ```bash
   git remote add upstream https://github.com/ORIGINAL_OWNER/alpha-c.git
   ```

4. **Create a development branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

5. **Build the project**:
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j$(nproc)
   ```

6. **Run tests**:
   ```bash
   ctest --verbose
   ```

## Coding Standards

This project follows the [LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html). Here are the key points:

### Code Style

- **Indentation**: 2 spaces, no tabs
- **Line Length**: 80 characters maximum
- **Naming Conventions**:
  - Classes: `CamelCase`
  - Functions: `camelBack`
  - Variables: `camelBack`
  - Constants: `kCamelCase`
  - Private members: `camelBack_`
  - Macros: `UPPER_CASE`

### Header Files

- Use `#pragma once` for header guards
- Include order: system headers, external library headers, project headers
- Forward declare when possible to reduce compile times

### Comments and Documentation

- Use `///` for Doxygen comments on public APIs
- Use `//` for regular comments
- Write self-documenting code with descriptive variable names
- Document complex algorithms and business logic

### Example Code Style

```cpp
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace alpha_compiler {

/// @brief Represents a lexical token in the Alpha-C language
class Token {
public:
  /// @brief Token types supported by the lexer
  enum class Type {
    kIdentifier,
    kNumber,
    kString,
    kOperator
  };

  /// @brief Construct a new Token
  /// @param type The token type
  /// @param value The token value
  /// @param line The source line number
  Token(Type type, std::string value, int line);

  /// @brief Get the token type
  /// @return The token type
  Type getType() const { return type_; }

  /// @brief Get the token value
  /// @return The token value as a string
  const std::string& getValue() const { return value_; }

private:
  Type type_;
  std::string value_;
  int line_;
};

} // namespace alpha_compiler
```

## Submitting Changes

### Branch Strategy

- **main**: Stable production branch
- **dev**: Development branch for ongoing work
- **feature/**: Feature branches for new functionality
- **hotfix/**: Hotfix branches for urgent fixes

### Commit Messages

Use clear, descriptive commit messages following this format:

```
type(scope): short description

Longer description if needed explaining what changed and why.

Fixes #issue_number
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or modifying tests
- `chore`: Build system, dependencies, etc.

### Pre-commit Checklist

Before submitting a pull request, ensure:

1. **Code is properly formatted**:
   ```bash
   clang-format -i src/**/*.cpp include/**/*.hpp
   ```

2. **Static analysis passes**:
   ```bash
   clang-tidy src/**/*.cpp -- -Iinclude -Iexternal
   ```

3. **All tests pass**:
   ```bash
   cd build && ctest --verbose
   ```

4. **Documentation is updated**:
   - Add Doxygen comments for new public APIs
   - Update README.md if necessary
   - Generate and review documentation: `doxygen Doxyfile`

5. **No compiler warnings**:
   ```bash
   cmake --build build -- -Wall -Wextra -Werror
   ```

## Testing

### Test Structure

- **Unit Tests**: Test individual components in isolation
- **Integration Tests**: Test interaction between components
- **End-to-End Tests**: Test complete compiler pipeline

### Adding Tests

1. Create test files in the `tests/` directory
2. Follow the naming convention: `test_<component>.cpp`
3. Include both positive and negative test cases
4. Test edge cases and error conditions
5. Ensure tests are deterministic and can run in parallel

### Test Example

```cpp
#include <gtest/gtest.h>
#include "lexer.hpp"

namespace alpha_compiler {
namespace test {

class LexerTest : public ::testing::Test {
protected:
  void SetUp() override {
    lexer_ = std::make_unique<Lexer>();
  }

  std::unique_ptr<Lexer> lexer_;
};

TEST_F(LexerTest, TokenizeIdentifier) {
  const std::string input = "variable_name";
  auto tokens = lexer_->tokenize(input);
  
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].getType(), Token::Type::kIdentifier);
  EXPECT_EQ(tokens[0].getValue(), "variable_name");
}

TEST_F(LexerTest, TokenizeNumber) {
  const std::string input = "42";
  auto tokens = lexer_->tokenize(input);
  
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].getType(), Token::Type::kNumber);
  EXPECT_EQ(tokens[0].getValue(), "42");
}

} // namespace test
} // namespace alpha_compiler
```

## Documentation

### API Documentation

- Document all public classes, methods, and functions
- Use Doxygen format for comments
- Include parameter descriptions and return values
- Provide usage examples where appropriate

### Code Documentation

- Comment complex algorithms and business logic
- Explain why decisions were made, not just what is done
- Keep comments up-to-date with code changes

### Building Documentation

```bash
doxygen Doxyfile
```

Documentation will be generated in `docs/html/index.html`.

## Issue Reporting

### Bug Reports

When reporting bugs, please include:

1. **Description**: Clear description of the problem
2. **Steps to Reproduce**: Detailed steps to reproduce the issue
3. **Expected Behavior**: What should happen
4. **Actual Behavior**: What actually happens
5. **Environment**: OS, compiler version, cmake version
6. **Code Sample**: Minimal code that reproduces the issue

### Feature Requests

When requesting features:

1. **Description**: Clear description of the feature
2. **Motivation**: Why this feature is needed
3. **Acceptance Criteria**: How to determine if the feature is complete
4. **Implementation Ideas**: Any thoughts on implementation

## Pull Request Process

### Before Submitting

1. **Sync with upstream**:
   ```bash
   git fetch upstream
   git rebase upstream/dev
   ```

2. **Run the full test suite**:
   ```bash
   cd build && ctest --verbose
   ```

3. **Check formatting and linting**:
   ```bash
   clang-format -i src/**/*.cpp include/**/*.hpp
   clang-tidy src/**/*.cpp -- -Iinclude -Iexternal
   ```

### Pull Request Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] All tests pass locally

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] No new compiler warnings
- [ ] CI checks pass
```

### Review Process

1. **Automated Checks**: CI pipeline must pass
2. **Code Review**: At least one maintainer review required
3. **Testing**: Reviewers will test the changes
4. **Documentation**: Ensure documentation is complete and accurate
5. **Approval**: Once approved, the PR will be merged

### After Merge

- **Update your fork**:
  ```bash
  git checkout dev
  git pull upstream dev
  git push origin dev
  ```

- **Delete feature branch**:
  ```bash
  git branch -d feature/your-feature-name
  git push origin --delete feature/your-feature-name
  ```

## Getting Help

- **GitHub Issues**: For bug reports and feature requests
- **GitHub Discussions**: For questions and general discussion
- **Documentation**: Check the project documentation first
- **Code Review**: Ask for help in pull request comments

Thank you for contributing to the Alpha-C Compiler project!