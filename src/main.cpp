#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cstring>
#include "ast.h"
#include "semantic.h"
#include "tac_gen.h"
#include "optimizer.h"
#include "error_handler.h"

extern int  yyparse();
extern FILE* yyin;
extern ProgramNode* rootAST;

FILE* tokenOut = nullptr;

static std::string stemOf(const std::string& path) {
    auto slash = path.find_last_of("/\\");
    std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
    auto dot = file.find_last_of('.');
    return (dot == std::string::npos) ? file : file.substr(0, dot);
}

struct Options {
    std::string inputFile;
    bool dumpTokens   = true;
    bool dumpAST      = true;
    bool dumpTAC      = true;
    bool dumpOpt      = true;
    bool benchmarkOpt = false;
};

static void usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " <source-file> [options]\n"
        << "  --no-tokens    suppress token dump\n"
        << "  --no-ast       suppress AST dump\n"
        << "  --no-tac       suppress TAC dump\n"
        << "  --no-opt       suppress optimised TAC dump\n"
        << "  --bench        print wall-clock time for optimiser\n";
}

static Options parseArgs(int argc, char** argv) {
    Options opts;
    if (argc < 2) { usage(argv[0]); std::exit(1); }
    opts.inputFile = argv[1];
    for (int i = 2; i < argc; ++i) {
        if      (std::strcmp(argv[i], "--no-tokens") == 0) opts.dumpTokens   = false;
        else if (std::strcmp(argv[i], "--no-ast")    == 0) opts.dumpAST      = false;
        else if (std::strcmp(argv[i], "--no-tac")    == 0) opts.dumpTAC      = false;
        else if (std::strcmp(argv[i], "--no-opt")    == 0) opts.dumpOpt      = false;
        else if (std::strcmp(argv[i], "--bench")     == 0) opts.benchmarkOpt = true;
        else { std::cerr << "Unknown option: " << argv[i] << "\n"; usage(argv[0]); std::exit(1); }
    }
    return opts;
}

int main(int argc, char** argv) {
    Options opts = parseArgs(argc, argv);

    std::filesystem::create_directories("output");
    const std::string stem = stemOf(opts.inputFile);

    yyin = std::fopen(opts.inputFile.c_str(), "r");
    if (!yyin) {
        std::cerr << "error: cannot open '" << opts.inputFile << "'\n";
        return 1;
    }

    if (opts.dumpTokens) {
        tokenOut = std::fopen(("output/" + stem + ".tokens").c_str(), "w");
        if (tokenOut)
            std::fprintf(tokenOut, "%-16s  %-24s  %s\n",
                         "TOKEN", "LEXEME", "LOCATION");
    }

    if (yyparse() != 0 || !rootAST) {
        std::cerr << "error: parsing failed.\n";
        if (tokenOut) { std::fclose(tokenOut); tokenOut = nullptr; }
        std::fclose(yyin);
        return 1;
    }
    if (tokenOut) { std::fclose(tokenOut); tokenOut = nullptr; }
    std::fclose(yyin);

    // ---- Semantic analysis ----
    SemanticAnalyzer sema;
    sema.analyze(rootAST);
    if (errorHandler.hasErrors()) {
        std::cerr << errorHandler.errorCount() << " error(s). Compilation aborted.\n";
        delete rootAST;
        return 1;
    }

    if (opts.dumpAST) {
        std::ofstream astOut("output/" + stem + ".ast");
        std::streambuf* prev = std::cout.rdbuf(astOut.rdbuf());
        rootAST->print();
        std::cout.rdbuf(prev);
    }

    // ---- IR Generation ----
    TACGenerator tacGen;
    tacGen.generate(rootAST);

    if (opts.dumpTAC) {
        std::ofstream tacOut("output/" + stem + ".tac");
        printTAC(tacGen.instructions, tacOut);
    }

    // ---- Optimisation ----
    TACProgram optimised = tacGen.instructions;

    auto t0 = std::chrono::steady_clock::now();
    Optimizer optimizer;
    optimizer.run(optimised);
    auto t1 = std::chrono::steady_clock::now();

    if (opts.benchmarkOpt) {
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::cout << "[bench] optimiser: " << ms << " ms\n";
    }

    if (opts.dumpOpt) {
        std::ofstream optOut("output/" + stem + ".opt.tac");
        printTAC(optimised, optOut);
    }

    std::cout << "Compilation successful. Output written to output/" << stem << ".*\n";

    delete rootAST;
    return 0;
}
