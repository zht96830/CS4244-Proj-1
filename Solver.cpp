#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <time.h>
#include <fstream>

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

    int num_clauses;        // number of clauses
    int num_variables;      // total number of variables
    int num_assigned;       // number of variables currently assigned
    int conflict_clause_number;    // clause that is found unsat, to be recorded for learning

    ReturnValue runCDCL();
    ReturnValue UnitPropagation(int decision_level);
    int pickBranchingVariable();
    void assignLiteral(int literal_to_make_true, int decision_level, int triggering_clause);
    void unassignVariable(int variable_to_unassign);
    int learnConflictAndBacktrack(int decision_level);
    vector<int> resolution(vector<int>& first_clause, int resolution_variable);
    int getVariableIndex(int literal);
    void printResult(ReturnValue result);

public: 
    /* intiailize class state from cin input.
     * 
    */
    void init();
    void solve();
};

// convert 1-indexed signed literal to 0-indexed unsigned variable
int CDCLSolver::getVariableIndex(int literal) {
    return abs(literal) - 1;
}

// Note: takes in a 1-indexed literal
// modifies variable_states vector for the corresponding 0-indexed variable
void CDCLSolver::assignLiteral(int literal_to_make_true, int decision_level, int triggering_clause) {
    int variable = getVariableIndex(literal_to_make_true);
    int polarity = (literal_to_make_true > 0) ? 1 : 0;
    variable_states[variable] = polarity;
    variable_assignment_decision_level[variable] = decision_level;
    variable_assignment_triggering_clause[variable] = triggering_clause;
    variable_frequency[variable] = -1;
    num_assigned++;
}

// Note: takes in a 0-indexed variable
// modifies variable_states vector for the corresponding 0-indexed variable
void CDCLSolver::unassignVariable(int variable_to_unassign) {
    variable_states[variable_to_unassign] = -1;
    variable_assignment_decision_level[variable_to_unassign] = -1;
    variable_assignment_triggering_clause[variable_to_unassign] = -1;
    variable_frequency[variable_to_unassign] = initial_variable_frequency[variable_to_unassign];
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
            if (unit_clause_exists) break;
            num_unassigned_in_clause = 0;
            num_false_in_clause = 0;
            is_clause_satisfied = false;

            for (int j = 0; j < formula[i].size(); j++) {
                int var = getVariableIndex(formula[i][j]);
                if (variable_states[var] == -1) {
                    // variable currently unassigned
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
                assignLiteral(formula[i][last_unassigned_literal], decision_level, i);
                break;
            } else if (num_false_in_clause == formula[i].size()) {
                // clause is unsat
                conflict_clause_number = i;
                return ReturnValue::unsat;
            }
        }
    } while (unit_clause_exists);
    // reset conflict clause to null if unit propagation succeeds
    conflict_clause_number == -1;
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
    vector<int> clause_to_learn = formula[conflict_clause_number];
    int num_literals_assigned_this_level = 0;
    // to be used later for resolution
    int resolution_variable = -1;

    // To count the number of variables assigned at this decision level
    while (true){
        num_literals_assigned_this_level = 0;
        for (int i = 0; i < clause_to_learn.size(); i++) {
            int variable = getVariableIndex(clause_to_learn[i]);
            
            if (variable_assignment_decision_level[variable] == decision_level) {
                num_literals_assigned_this_level++;
                
                // if the assignment was triggered by unit propagation, save the variable for resolution
                if (variable_assignment_triggering_clause[variable] != -1) {
                    resolution_variable = variable;
                }
            }
        }
        // if there is only 1 literal in the conflict causing clause assigned this level,
        // it must be a root of the implication graph. i.e. ready to cut
        if (num_literals_assigned_this_level == 1) break;
        // otherwise, apply resolution on the currently related clauses
        // to propagate up the implication graph
        clause_to_learn = resolution(clause_to_learn, resolution_variable);
    }

    // learn clause and update states
    formula.push_back(clause_to_learn);
    for (int i = 0; i < clause_to_learn.size(); i++)  {
        int variable = getVariableIndex(clause_to_learn[i]);
        if (clause_to_learn[i] > 0) {
            literal_polarity_difference[variable]++;
        } else {
            literal_polarity_difference[variable]--;
        }
        initial_variable_frequency[variable]++;
        // if variable has not been assigned, update current frequency
        if (variable_states[variable] == -1) {
            variable_frequency[variable]++;
        }
    }
    // update current number of clauses
    num_clauses = formula.size();
    
    // determining backtracking decision level
    // find max level where literal in learnt clause has been assigned that is not current level
    int decision_level_to_backtrack = 0;
    for (int i = 0; i < clause_to_learn.size(); i++) {
        int possible_decision_level = variable_assignment_decision_level[getVariableIndex(clause_to_learn[i])];
        if (possible_decision_level < decision_level && 
            possible_decision_level > decision_level_to_backtrack) {
            decision_level_to_backtrack = possible_decision_level;
        }
    }
    // unassign all variables post-backtracking level
    for (int i = 0; i < variable_states.size(); i++) {
        if (variable_assignment_decision_level[i] > decision_level_to_backtrack) {
            unassignVariable(i);
        }
    }
    return decision_level_to_backtrack;
}

vector<int> CDCLSolver::resolution(vector<int>& first_clause, int resolution_variable) {
    vector<int> second_clause = formula[variable_assignment_triggering_clause[resolution_variable]];
    // combine the two clauses
    first_clause.insert(first_clause.end(), second_clause.begin(), second_clause.end());

    // remove any literals of the resolution variable
    for (int i = 0; i < first_clause.size(); i++) {
        if (first_clause[i] == resolution_variable + 1 || first_clause[i] == -resolution_variable - 1) {
            first_clause.erase(first_clause.begin() + i);
            i--;
        }
    }
    // remove duplicates
    sort(first_clause.begin(), first_clause.end() );
    first_clause.erase( unique( first_clause.begin(), first_clause.end() ), first_clause.end() );

    return first_clause;
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
        // cout << "num_assigned: " << num_assigned << " num_variables: " << num_variables << endl;
        // pick a variable to assign
        int literal_to_make_true = pickBranchingVariable();
        decision_level++;
        assignLiteral(literal_to_make_true, decision_level, -1);

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

            // cout << "backtracked_decision_level: " << decision_level << endl;
        }

        // if unit propagation finishes without discovering UNSAT, continue to pick next variable
    }
    // after all variables have been assigned, return SAT
    return ReturnValue::sat;
}

void CDCLSolver::init() {
    char c;
    string s;
    // ignore comments
    while (true) {
        cin >> c;
        if (c == 'c') {
            getline(cin, s);            
        } else {
            // should be == 'p'
            break;
        }
    }
    cin >> s;
    cin >> num_variables;
    cin >> num_clauses;

    // reset class variables
    conflict_clause_number = -1;
    num_assigned = 0;

    // reset vectors
    formula.clear();
    formula.resize(num_clauses);
    variable_states.clear();
    variable_states.resize(num_variables, -1);
    variable_assignment_decision_level.clear();
    variable_assignment_decision_level.resize(num_variables, -1);
    variable_assignment_triggering_clause.clear();
    variable_assignment_triggering_clause.resize(num_variables, -1);
    variable_frequency.clear();
    variable_frequency.resize(num_variables, 0);
    literal_polarity_difference.clear();
    literal_polarity_difference.resize(num_variables, 0);

    int literal;

    for (int i = 0; i < num_clauses; i++) {
        while (true) {
            cin >> literal;
            int variable = getVariableIndex(literal);
            if (literal > 0) {
                formula[i].push_back(literal);
                variable_frequency[variable]++;
                literal_polarity_difference[variable]++;
            } else if (literal < 0) {
                formula[i].push_back(literal);
                variable_frequency[variable]++;
                literal_polarity_difference[variable]--;
            } else {
                // end of claused reached
                break;
            }
        }
    }
    // make a copy of variable frequency at initialization
    initial_variable_frequency = variable_frequency;
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
            cout << ((variable_states[i] == 1) ? "" : "-") << i+1 << " ";
        }
        cout << "0" << endl;
    } else {
        // print UNSAT
        cout << "UNSAT" << endl;
    }
}

int main()
{
    // open file
    ofstream timefile;
    timefile.open ("time2.txt");

    CDCLSolver solver;
    solver.init();
    
    // measure time start
    clock_t t;
	t = clock();

    solver.solve();
    // measure time end
	clock_t timeTaken = clock() - t;
	// cout << "time: " << t << " miliseconds" << endl;
	// cout << CLOCKS_PER_SEC << " clocks per second" << endl;
	// cout << "time: " << t*1.0/CLOCKS_PER_SEC << " seconds" << endl;

    // double dif = difftime (end,start);
    cout << timeTaken << endl;
    timefile << timeTaken << "\n";
    
    timefile.close();
    return 0;
}