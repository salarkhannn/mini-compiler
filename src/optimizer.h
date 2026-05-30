#pragma once

#include "tac.h"

class Optimizer {
    void constantFoldAndPropagate(TACProgram& prog);
    void commonSubexpressionElimination(TACProgram& prog);
    void deadCodeElimination(TACProgram& prog);
    void unreachableBlockElimination(TACProgram& prog);
    void loopInvariantCodeMotion(TACProgram& prog);

public:
    void run(TACProgram& prog);
};
