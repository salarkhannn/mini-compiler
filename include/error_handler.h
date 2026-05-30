#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

enum class ErrorPhase { LEXICAL, SYNTAX, SEMANTIC, IR, OPTIMIZER };

struct Diagnostic {
    ErrorPhase  phase;
    int         line;
    int         col;
    std::string message;
};

class ErrorHandler {
    std::vector<Diagnostic> diags;

public:
    void report(ErrorPhase phase, int line, int col, const std::string& msg) {
        diags.push_back({phase, line, col, msg});
        std::string prefix;
        switch (phase) {
            case ErrorPhase::LEXICAL:   prefix = "lexical";   break;
            case ErrorPhase::SYNTAX:    prefix = "syntax";    break;
            case ErrorPhase::SEMANTIC:  prefix = "semantic";  break;
            case ErrorPhase::IR:        prefix = "ir";        break;
            case ErrorPhase::OPTIMIZER: prefix = "optimizer"; break;
        }
        std::cerr << "[" << prefix << " error] line " << line;
        if (col > 0) std::cerr << ":" << col;
        std::cerr << ": " << msg << "\n";
    }

    bool hasErrors() const { return !diags.empty(); }

    size_t errorCount() const { return diags.size(); }

    const std::vector<Diagnostic>& diagnostics() const { return diags; }
};

extern ErrorHandler errorHandler;
