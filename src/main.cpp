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

#define MC_VERSION "0.1.0"

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
    bool showStats    = false;
    bool showVersion  = false;
    bool showHelp     = false;
};

static void usage(const char* prog) {
    std::cout
        << "Usage: " << prog << " <source-file> [options]\n"
        << "\nOptions:\n"
        << "  --no-tokens    Suppress token dump\n"
        << "  --no-ast       Suppress AST dump\n"
        << "  --no-tac       Suppress unoptimised TAC dump\n"
        << "  --no-opt       Suppress optimised TAC dump\n"
        << "  --bench        Print optimizer wall-clock time\n"
        << "  --stats        Print instruction count before/after optimization\n"
        << "  --version      Print version and exit\n"
        << "  --help         Print this message and exit\n";
}

static Options parseArgs(int argc, char** argv) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        if      (std::strcmp(argv[i], "--help")      == 0) opts.showHelp     = true;
        else if (std::strcmp(argv[i], "--version")   == 0) opts.showVersion  = true;
        else if (std::strcmp(argv[i], "--no-tokens") == 0) opts.dumpTokens   = false;
        else if (std::strcmp(argv[i], "--no-ast")    == 0) opts.dumpAST      = false;
        else if (std::strcmp(argv[i], "--no-tac")    == 0) opts.dumpTAC      = false;
        else if (std::strcmp(argv[i], "--no-opt")    == 0) opts.dumpOpt      = false;
        else if (std::strcmp(argv[i], "--bench")     == 0) opts.benchmarkOpt = true;
        else if (std::strcmp(argv[i], "--stats")     == 0) opts.showStats    = true;
        else if (argv[i][0] == '-') {
            std::cerr << "error: unknown option '" << argv[i] << "'\n";
            usage(argv[0]);
            std::exit(1);
        } else {
            if (!opts.inputFile.empty()) {
                std::cerr << "error: unexpected argument '" << argv[i] << "'\n";
                std::exit(1);
            }
            opts.inputFile = argv[i];
        }
    }
    if (opts.showHelp)    { usage(argv[0]); std::exit(0); }
    if (opts.showVersion) { std::cout << "mc version " << MC_VERSION << "\n"; std::exit(0); }
    if (opts.inputFile.empty()) { usage(argv[0]); std::exit(1); }
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
    size_t beforeCount = 0;
    size_t afterCount  = 0;
    if (opts.showStats) {
        for (const auto& inst : tacGen.instructions)
            if (inst.op != TACOp::LABEL) ++beforeCount;
    }

    auto t0 = std::chrono::steady_clock::now();
    Optimizer optimizer;
    optimizer.run(optimised);
    auto t1 = std::chrono::steady_clock::now();

    if (opts.showStats) {
        for (const auto& inst : optimised)
            if (inst.op != TACOp::LABEL) ++afterCount;
        double reduction = (beforeCount > 0)
            ? (1.0 - static_cast<double>(afterCount) / beforeCount) * 100.0
            : 0.0;
        std::cout << "[stats] instructions: " << beforeCount << " -> " << afterCount
                  << " (" << reduction << "% reduction)\n";
    }

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
