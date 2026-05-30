#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "ast.h"

struct Symbol {
    std::string       name;
    Type              type;
    int               scopeLevel;
    int               lineDeclared;
    bool              isFunction;
    std::vector<Type> paramTypes;
};

class SymbolTable {
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    int level = 0;

public:
    SymbolTable() { scopes.emplace_back(); }

    void enterScope() {
        ++level;
        scopes.emplace_back();
    }

    void exitScope() {
        if (scopes.size() > 1) {
            scopes.pop_back();
            --level;
        }
    }

    bool insert(const std::string& name, Type type, int lineDeclared,
                bool isFunction = false, const std::vector<Type>& paramTypes = {}) {
        auto& frame = scopes.back();
        if (frame.count(name)) return false;
        frame[name] = {name, type, level, lineDeclared, isFunction, paramTypes};
        return true;
    }

    Symbol* lookup(const std::string& name) {
        for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end()) return &it->second;
        }
        return nullptr;
    }

    Symbol* lookupCurrentScope(const std::string& name) {
        auto it = scopes.back().find(name);
        return (it != scopes.back().end()) ? &it->second : nullptr;
    }

    int currentLevel() const { return level; }
};
