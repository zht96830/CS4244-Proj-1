#include <iostream>
#include <random>
#include <vector>
#include <cmath>

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
     * indexed from 0. Literals are indexed from 1;
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

    // difference between number of true literals and false literals
    vector<int> literal_polarity_difference;

    // stores for each variable which level in CDCL it is assigned
    vector<int> variable_assignment_decision_level;

    // marks the clause number that forced this assignment
    // if variable is picked, mark -1 instead
    vector<int> variable_assignment_triggering_clause;

    int num_variables;      // total number of variables
    int num_assigned;       // number of variables currently assigned
    int conflict_clause;    // clause that is found unsat, to be recorded for learning

    ReturnValue runCDCL();
    ReturnValue UnitPropagation(int decision_level);
    int pickBranchingVariable();
    void assignVariable(int literal_to_make_true, int decision_level, int triggering_clause);
    void unassignVariable(int literal_to_unassign);
    int learnConflictAndBacktrack(int decision_level);
    int getVariableIndex(int literal);

    void printResult(ReturnValue result);

public: 
    /* intiailize class state from cin input.
     * 
    */
    void init();
    void solve();
};

int getVariableIndex(int literal) {
    return abs(literal) - 1;
}

void CDCLSolver::assignVariable(int literal_to_make_true, int decision_level, int triggering_clause) {
    int variable = abs(literal_to_make_true);
    int polarity = literal_to_make_true > 0 ? 1 : 0;
    variable_states[variable] = polarity;
    variable_assignment_decision_level[variable] = decision_level;
    variable_assignment_triggering_clause[variable] = triggering_clause;
    variable_frequency[variable] = -1;
    num_assigned++;
}

void CDCLSolver::unassignVariable(int literal_to_unassign) {
    variable_states[literal_to_unassign] = -1;
    variable_assignment_decision_level[literal_to_unassign] = -1;
    variable_assignment_triggering_clause[literal_to_unassign] = -1;
    variable_frequency[literal_to_unassign] = initial_variable_frequency[literal_to_unassign];
    num_assigned--;
}

ReturnValue CDCLSolver::UnitPropagation(int decision_level) {
    bool unit_clause_exists = false;
    int num_unassigned_in_clause = 0;
    int num_false_in_clause = 0;
    int last_unassigned_literal = -1;
    bool is_clause_satisfied = false;

    do {
        unit_clause_exists = false;
        
        // for each clause, count number of unassigned literals within
        for (int i = 0; i < formula.size(); i++) {
            num_unassigned_in_clause = 0;
            num_false_in_clause = 0;
            last_unassigned_literal = -1;
            is_clause_satisfied = false;

            for (int j = 0; j< formula[i].size(); j++) {
                int var = getVariableIndex(formula[i][j]);
                if (variable_states[var] == -1) {
                    // currently unassigned
                    num_unassigned_in_clause++;
                    last_unassigned_literal = j;
                } else if ((variable_states[var] == 0 && formula[i][j] > 0)
                || (variable_states[var] == 1 && formula[i][j] < 0)) {
                    // literal is false given current state of assignments
                    num_false_in_clause++;
                } else {
                    // literal is true given current state of assignments
                    is_clause_satisfied = true;
                    break;
                }   
            }
            if (is_clause_satisfied) {
                // move on to the next clause;
                continue;
            }
            if (num_unassigned_in_clause == 1) {
                // Unit clause found
                unit_clause_exists = true;
                assignVariable(last_unassigned_literal, decision_level, i);
                break;
            } else if (num_false_in_clause == formula[i].size()) {
                // clause is unsat
                conflict_clause = i;
                return ReturnValue::unsat;
            }
        }
    } while (unit_clause_exists);
    // reset conflict clause to null if unit propagation succeeds
    conflict_clause == -1;
    return ReturnValue::normal;
}

// returns a literal to be assigned true with sign (+/-) representing polarity
// note: literal is 1-indexed
// currently just picks variable with highest frequency, and chooses
// the most frequent polarity to assign true
int CDCLSolver::pickBranchingVariable() {
    int max_frequency = 0;
    int max_frequency_variable = -1;
    for (int i = 0; i < variable_frequency.size(); i++) {
        if (variable_frequency[i] > max_frequency) {
            max_frequency = variable_frequency[i];
            max_frequency_variable = i;
        }
    }
    if (literal_polarity_difference[max_frequency_variable] < 0) {
        // there are more false literals in the formula currently
        // return the literal to be assigned true
        return -max_frequency_variable - 1;
    } 
    return max_frequency_variable + 1;
}

int CDCLSolver::learnConflictAndBacktrack(int decision_level){
    
}

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
        // pick a variable to assign
        int literal_to_make_true = pickBranchingVariable();
        decision_level++;
        assignVariable(literal_to_make_true, decision_level, -1);

        // unit propagate | generate implication graph to check for unsat
        up_result = UnitPropagation(decision_level);

        while (up_result == ReturnValue::unsat) {
            // Shortcircuit: If at any moment after learning some clauses and jumping back to 
            // decision lvl 0 we realize through unit propagation the formula is unsat,
            // return unsat
            if (decision_level == 0) return up_result;
            
            // otherwise learn new clause then backtrack
            decision_level = learnConflictAndBacktrack(decision_level);

            // unit propagate for again
            up_result = UnitPropagation(decision_level);
        }

        // if unit propagation finishes without discovering UNSAT, continue to pick next variable
    }
    // after all variables have been assigned, return SAT
    return ReturnValue::sat;
}

void CDCLSolver::init() {
}

void CDCLSolver::solve() {
    ReturnValue result = runCDCL();
    printResult(result);
}

void CDCLSolver::printResult(ReturnValue result) {
    if (result == ReturnValue::sat) {
        cout << "SAT" << endl;
        for (int i = 0; i < num_variables; i++) {
            // for variables that are assigned true, print as true;
            // for unassigned variables (which at this stage can take any value), print as false 
            cout << ((variable_states[i] > 0) ? "" : "-") << i+1 << " ";
        }
        cout << "0" << endl;
    } else {
        // print UNSAT
        cout << "UNSAT" << endl;
    }
}


int main()
{
    CDCLSolver solver;
    solver.init();
    solver.solve();
    return 0;
}
