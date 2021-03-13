#include <iostream>
#include <random>
#include <vector>

using namespace std;

enum ReturnValue
{
    sat,   // formula is satisfiable
    unsat, // formula is unsatisfiable
    normal // formula satisfiability undetermined
};

class CDCLSolver
{
    /* stores info on whether variable has been assigned
     * -1: unassigned
     * 0 : assigned false
     * 1 : assigned true
    */
    vector<int> variable_states;

    // the given 3CNF
    vector<vector<int>> formula;

    // to be used for variable picking
    vector<int> variable_frequency;

    // to be used for resetting
    vector<int> initial_variable_frequency;

    // variable polarity difference 
    vector<int> variable_polarity_difference;

    // stores for each variable which level in CDCL it is assigned
    vector<int> variable_assignment_decision_level;

    // marks the clause number that forced this assignment
    // if variable is picked, mark -1 instead
    vector<int> literal_antecedent;

    int num_variables;      // total number of variables
    int num_assigned;       // number of variables currently assigned

    ReturnValue runCDCL();
    ReturnValue UnitPropagation(int decision_level);
    int pickBranchingVariable();
    void assignVariable(int literal_to_assign, int decision_level, int triggering_clause);

public: 
    /* intiailize class state from cin input.
     * 
    */
    void init();
    void solve();
};


ReturnValue CDCLSolver::UnitPropagation(int decision_level) {

}

// returns an int with magnitude representing a variable number,
// and sign (+/-) representing whether to assign true or false
int CDCLSolver::pickBranchingVariable() {}

ReturnValue CDCLSolver::runCDCL() {
    int decision_level = 0;

    // -------------------------
    // Edge case checking / short circuiting:
    // -------------------------

    // check if initialized formula has no clauses: return sat
    if (formula.size() == 0) return ReturnValue::sat;
    // if initialized formula has empty clauses: return unsat
    for (int i = 0; i < formula.size(); i++) {
        if (formula[i].size() == 0) return ReturnValue::unsat;
    }
    // Unit propagation
    ReturnValue up_result = UnitPropagation(decision_level);
    if (up_result == ReturnValue::unsat) return up_result;

    // -------------------------
    // Now entering CDCL Main Loop
    // -------------------------
    
    // while not all variables are assigned: 
    while (num_assigned != num_variables) {
        int literal_to_assign = pickBranchingVariable();
        decision_level++;

        assignVariable(literal_to_assign, decision_level, -1);

        while (true) {
            // 		UnitPropResult = UnitPropLoop()
            // 		If (UnitPropResult == UNSAT ) ConflictAnalysis&Backtrack()
            // 		Else break // back to pickBranchingVar
        }
    }
    return ReturnValue::sat;
}

void CDCLSolver::init() {

}

void CDCLSolver::solve() {
}

int main()
{
    CDCLSolver solver;
    solver.init();
    solver.solve();
    return 0;
}
