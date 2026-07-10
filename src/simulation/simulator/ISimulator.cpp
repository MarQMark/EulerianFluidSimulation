#include "simulation/simulator/ISimulator.h"

#include "simulation/Properties.h"

bool ISimulator::availableSolvers(const std::vector<Properties::SolverType>& solvers) {
    std::vector<Properties::SolverType>* avblSolvers = PROPERTY(AVLB_SOLVERS, std::vector<Properties::SolverType>*);
    avblSolvers->clear();
    for (auto s : solvers)
        avblSolvers->push_back(s);

    auto solver = PROPERTY(SOLVER_TYPE, Properties::SolverType);
    bool validSolver = false;
    for (const auto s : *avblSolvers) {
        if (s == solver) {
            validSolver = true;
            break;
        }
    }
    if (!validSolver) {
        if (avblSolvers->empty()) {
            LOG_ERROR("No available solver for simulation type")
        }
        else {
            solver = (*avblSolvers)[0];
            validSolver = true;
        }
    }

    if (validSolver)
        SET_PROPERTY(SOLVER_TYPE, solver, Properties::SolverType);

    return validSolver;
}
