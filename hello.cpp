#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <algorithm>
#include <filesystem>
#include <iomanip>

using namespace std;

/*
 Reworked LR(0) parser generator and parser.
 - Parses grammar lines like "S -> A a" or "S->A a" (rhs tokens separated by whitespace).
 - Augments grammar with S' -> S (production index 0).
 - Represents items as (prodIndex, dotPos).
 - Builds closure, goto, canonical collection.
 - Builds ACTION (shift/reduce/accept) and GOTO tables.
 - Emits conflicts to cerr.
 - Parses an input token sequence (tokens separated by whitespace).
 - Saves parse trace to parsable_strings/{grammar_id}/{compressed_input}.txt
*/

struct Production {
    string lhs;
    vector<string> rhs;
    string to_string() const {
        string s = lhs + "->";
        for (size_t i = 0; i < rhs.size(); ++i) {
            if (i) s += " ";
            s += rhs[i];
        }
        return s;
    }
};

struct Item {
    int prod;   // production index
    int dot;    // position of dot (0..rhs.size())
    bool operator<(Item const& o) const {
        if (prod != o.prod) return prod < o.prod;
        return dot < o.dot;
    }
    bool operator==(Item const& o) const {
        return prod == o.prod && dot == o.dot;
    }
};

static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static vector<string> split_ws(const string& s) {
    vector<string> out;
    string token;
    stringstream ss(s);
    while (ss >> token) out.push_back(token);
    return out;
}

string vector_key_items(const vector<Item>& v) {
    // produce canonical key for set of items (sorted)
    vector<Item> tmp = v;
    sort(tmp.begin(), tmp.end());
    string out;
    for (size_t i = 0; i < tmp.size(); ++i) {
        if (i) out += "|";
        out += to_string(tmp[i].prod) + ":" + to_string(tmp[i].dot);
    }
    return out;
}

vector<Item> closure_items(const vector<Item>& items, const vector<Production>& prods, const unordered_map<string, vector<int>>& lhs_to_prods) {
    vector<Item> res = items;
    // use set to avoid duplicates
    set<pair<int,int>> seen;
    for (const Item &it: res) seen.insert({it.prod, it.dot});
    for (size_t idx = 0; idx < res.size(); ++idx) {
        Item cur = res[idx];
        const Production& P = prods[cur.prod];
        if (cur.dot < (int)P.rhs.size()) {
            string B = P.rhs[cur.dot];
            // If B is a nonterminal (present as lhs in grammar)
            if (lhs_to_prods.count(B)) {
                for (int pidx : lhs_to_prods.at(B)) {
                    pair<int,int> key = {pidx, 0};
                    if (!seen.count(key)) {
                        seen.insert(key);
                        res.push_back(Item{pidx, 0});
                    }
                }
            }
        }
    }
    return res;
}

vector<Item> goto_items(const vector<Item>& items, const string& symbol, const vector<Production>& prods, const unordered_map<string, vector<int>>& lhs_to_prods) {
    vector<Item> moved;
    for (const Item& it : items) {
        const Production& P = prods[it.prod];
        if (it.dot < (int)P.rhs.size() && P.rhs[it.dot] == symbol) {
            moved.push_back(Item{it.prod, it.dot + 1});
        }
    }
    if (moved.empty()) return {};
    return closure_items(moved, prods, lhs_to_prods);
}

string compress_name(const string& name) {
    unordered_map<char, int> freq;
    for (char c : name) if (!isspace((unsigned char)c)) freq[c]++;
    string out;
    for (const auto& kv : freq) {
        out.push_back(kv.first);
        out += to_string(kv.second);
    }
    return out;
}

void save_result_file(const string& content, int grammar_id, const string& filename) {
    string dir = "parsable_strings/" + to_string(grammar_id) + "/";
    std::filesystem::create_directories(dir);
    ofstream ofs(dir + filename + ".txt");
    ofs << content;
    ofs.close();
}

string join_stack(const vector<string>& st) {
    string s;
    for (size_t i = 0; i < st.size(); ++i) {
        if (i) s += " ";
        s += st[i];
    }
    return s;
}

string table_to_string(const vector<vector<string>>& table, const vector<string>& header) {
    vector<size_t> widths;
    size_t cols = header.size();
    widths.assign(cols, 0);
    for (size_t j = 0; j < cols; ++j) widths[j] = header[j].size();
    for (const auto& row : table) {
        for (size_t j = 0; j < cols && j < row.size(); ++j) {
            widths[j] = max(widths[j], row[j].size());
        }
    }
    stringstream ss;
    // header
    for (size_t j = 0; j < cols; ++j) {
        ss << "|" << string(widths[j] - header[j].size(), ' ') << header[j] << " ";
    }
    ss << "|\n";
    // separator
    for (size_t j = 0; j < cols; ++j) {
        ss << "+" << string(widths[j] + 1, '-');
    }
    ss << "+\n";
    for (const auto& row : table) {
        for (size_t j = 0; j < cols; ++j) {
            string cell = j < row.size() ? row[j] : "";
            ss << "|" << string(widths[j] - cell.size(), ' ') << cell << " ";
        }
        ss << "|\n";
    }
    return ss.str();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "LR(0) Parser Generator & Parser\n\n";

    int grammar_id;
    cout << "Enter grammar number: ";
    if (!(cin >> grammar_id)) return 1;
    cout << "\n";

    string grammar_path = "grammar/" + to_string(grammar_id) + ".txt";
    ifstream gfile(grammar_path);
    if (!gfile.is_open()) {
        cerr << "Cannot open grammar file: " << grammar_path << endl;
        return 1;
    }

    vector<Production> prods; // will include augmented prod at index 0
    vector<string> raw_lines;
    string line;
    while (getline(gfile, line)) {
        line = trim(line);
        if (!line.empty()) raw_lines.push_back(line);
    }
    gfile.close();

    if (raw_lines.empty()) {
        cerr << "Grammar file is empty\n";
        return 1;
    }

    // Parse productions from file
    for (const string& l : raw_lines) {
        size_t pos = l.find("->");
        if (pos == string::npos) pos = l.find("->"); // keep legacy
        if (pos == string::npos) {
            cerr << "Skipping invalid line (no ->): " << l << "\n";
            continue;
        }
        string lhs = trim(l.substr(0, pos));
        string rhs_str = trim(l.substr(pos + 2));
        vector<string> rhs_tokens = split_ws(rhs_str);
        // If RHS is empty, keep empty vector (epsilon)
        Production P;
        P.lhs = lhs;
        P.rhs = rhs_tokens;
        prods.push_back(P);
    }

    // Determine start symbol as LHS of first production in file
    string start_symbol = prods.front().lhs;
    // Augment grammar S' -> start_symbol
    Production aug;
    aug.lhs = "S'";
    aug.rhs = { start_symbol };
    // Insert at beginning
    prods.insert(prods.begin(), aug);

    // Build helper maps
    unordered_map<string, vector<int>> lhs_to_prods;
    for (size_t i = 0; i < prods.size(); ++i) {
        lhs_to_prods[prods[i].lhs].push_back((int)i);
    }

    // Determine terminals and nonterminals
    set<string> nonterms_set;
    for (const auto& p : prods) nonterms_set.insert(p.lhs);
    set<string> terms_set;
    for (const auto& p : prods) {
        for (const string& tok : p.rhs) {
            if (!nonterms_set.count(tok)) terms_set.insert(tok);
        }
    }
    terms_set.insert("$");
    vector<string> nonterms(nonterms_set.begin(), nonterms_set.end());
    vector<string> terminals(terms_set.begin(), terms_set.end());

    // Build canonical LR(0) collection
    vector<vector<Item>> states;
    unordered_map<string,int> state_id_by_key;
    map<int, map<string,int>> transitions; // state -> symbol -> state

    // state 0: closure of item (prod 0, dot 0)
    vector<Item> start_items = closure_items({ Item{0,0} }, prods, lhs_to_prods);
    states.push_back(start_items);
    state_id_by_key[vector_key_items(start_items)] = 0;

    for (size_t si = 0; si < states.size(); ++si) {
        // collect all symbols that appear right after dot in items
        set<string> symbols;
        for (const Item& it : states[si]) {
            const Production& P = prods[it.prod];
            if (it.dot < (int)P.rhs.size()) {
                symbols.insert(P.rhs[it.dot]);
            }
        }
        for (const string& sym : symbols) {
            vector<Item> gotoSet = goto_items(states[si], sym, prods, lhs_to_prods);
            if (gotoSet.empty()) continue;
            string key = vector_key_items(gotoSet);
            int nid;
            if (!state_id_by_key.count(key)) {
                nid = (int)states.size();
                states.push_back(gotoSet);
                state_id_by_key[key] = nid;
            } else {
                nid = state_id_by_key[key];
            }
            transitions[(int)si][sym] = nid;
        }
    }

    // Print states
    cout << "---------------------------------------------------------------\n";
    cout << "Total States: " << states.size() << "\n";
    for (size_t i = 0; i < states.size(); ++i) {
        cout << i << " : [";
        for (size_t j = 0; j < states[i].size(); ++j) {
            if (j) cout << ", ";
            const Item& it = states[i][j];
            const Production& P = prods[it.prod];
            // print with dot
            cout << P.lhs << "->";
            for (size_t k = 0; k < P.rhs.size(); ++k) {
                if ((int)k == it.dot) cout << ".";
                if (k) cout << " ";
                cout << P.rhs[k];
            }
            if (it.dot == (int)P.rhs.size()) cout << ".";
        }
        cout << "]\n";
    }
    cout << "---------------------------------------------------------------\n";

    // Build ACTION and GOTO tables
    // ACTION: map<int state, map<string terminal, string action>>
    // GOTO: map<int state, map<string nonterm, int>>
    map<int, map<string, string>> ACTION;
    map<int, map<string, int>> GOTO;

    // Fill shifts and gotos from transitions
    for (const auto& stkv : transitions) {
        int s = stkv.first;
        for (const auto& symkv : stkv.second) {
            const string& sym = symkv.first;
            int tgt = symkv.second;
            if (nonterms_set.count(sym)) {
                GOTO[s][sym] = tgt;
            } else {
                ACTION[s][sym] = "S" + to_string(tgt);
            }
        }
    }

    // Fill reductions and accept
    for (size_t s = 0; s < states.size(); ++s) {
        for (const Item& it : states[s]) {
            const Production& P = prods[it.prod];
            if (it.dot == (int)P.rhs.size()) {
                // dot at the end -> reduce or accept
                if (it.prod == 0) {
                    // augmented production S' -> S .
                    ACTION[(int)s]["$"] = "Accept";
                } else {
                    // reduce by production it.prod
                    string action = "r" + to_string((int)it.prod);
                    // LR(0): reduction applies on all terminals
                    for (const string& t : terminals) {
                        // don't overwrite existing shift/action unless conflict
                        if (ACTION[(int)s].count(t)) {
                            // Conflict: existing action (shift or reduce)
                            string existing = ACTION[(int)s][t];
                            if (existing != action) {
                                cerr << "Conflict at state " << s << " on symbol '" << t << "': existing='" << existing << "' new='" << action << "'\n";
                                // prefer shift (conservative) — do not overwrite shift with reduce
                                // keep existing
                            }
                        } else {
                            ACTION[(int)s][t] = action;
                        }
                    }
                }
            }
        }
    }

    // Build printing table rows
    vector<string> header;
    header.push_back("State");
    for (const string& t : terminals) header.push_back(t);
    for (const string& nt : nonterms) {
        header.push_back(nt);
    }

    vector<vector<string>> table_rows;
    for (size_t s = 0; s < states.size(); ++s) {
        vector<string> row;
        row.push_back(to_string((int)s));
        row.resize(1 + terminals.size() + nonterms.size(), "");
        // fill terminals (ACTION)
        for (size_t ti = 0; ti < terminals.size(); ++ti) {
            const string& t = terminals[ti];
            if (ACTION.count((int)s) && ACTION[(int)s].count(t)) row[1 + ti] = ACTION[(int)s][t];
        }
        // fill nonterminals (GOTO)
        for (size_t nti = 0; nti < nonterms.size(); ++nti) {
            const string& nt = nonterms[nti];
            size_t idx = 1 + terminals.size() + nti;
            if (GOTO.count((int)s) && GOTO[(int)s].count(nt)) row[idx] = to_string(GOTO[(int)s][nt]);
        }
        table_rows.push_back(row);
    }

    // Print table
    cout << "\nParsing Table:\n\n";
    cout << table_to_string(table_rows, header) << "\n";

    // Map production index to textual form and to lhs/rhs for parsing
    unordered_map<int, Production> prod_by_index;
    for (size_t i = 0; i < prods.size(); ++i) prod_by_index[(int)i] = prods[i];

    // Input string parsing
    cout << "\nEnter the string to be parsed (tokens separated by spaces): ";
    string input_line;
    cin >> ws;
    getline(cin, input_line);
    vector<string> input_tokens;
    if (!trim(input_line).empty()) {
        input_tokens = split_ws(trim(input_line));
    }
    input_tokens.push_back("$");

    // Parse with table
    bool accepted = false;
    vector<vector<string>> trace;
    vector<string> stack_symbols; // alternates: state, symbol, state, ...
    stack_symbols.push_back("0"); // initial state

    int pointer = 0;
    while (true) {
        if (stack_symbols.empty()) break;
        int state = stoi(stack_symbols.back());
        string lookahead = (pointer < (int)input_tokens.size()) ? input_tokens[pointer] : "$";
        string action = "";
        if (ACTION.count(state) && ACTION[state].count(lookahead)) action = ACTION[state][lookahead];
        else {
            // No direct action -> error
            break;
        }

        if (action == "Accept") {
            trace.push_back({ "Action(" + to_string(state) + ", " + lookahead + ") = Accept",
                              to_string(pointer), lookahead, join_stack(stack_symbols) });
            accepted = true;
            break;
        } else if (!action.empty() && action[0] == 'S') {
            int nextState = stoi(action.substr(1));
            trace.push_back({ "Action(" + to_string(state) + ", " + lookahead + ") = " + action,
                              to_string(pointer), lookahead, join_stack(stack_symbols) });
            // push symbol and state
            stack_symbols.push_back(lookahead);
            stack_symbols.push_back(to_string(nextState));
            pointer++;
        } else if (!action.empty() && action[0] == 'r') {
            int prodIndex = stoi(action.substr(1));
            Production P = prod_by_index[prodIndex];
            // pop 2 * |rhs| entries
            int popCount = 2 * (int)P.rhs.size();
            for (int i = 0; i < popCount; ++i) {
                if (!stack_symbols.empty()) stack_symbols.pop_back();
            }
            // push lhs
            stack_symbols.push_back(P.lhs);
            // get goto from previous state
            if (stack_symbols.size() < 2) {
                // invalid
                break;
            }
            int prevState = stoi(stack_symbols[stack_symbols.size() - 2]);
            string gotoVal = "";
            if (GOTO.count(prevState) && GOTO[prevState].count(P.lhs)) {
                int gt = GOTO[prevState][P.lhs];
                gotoVal = to_string(gt);
            } else {
                // error
                break;
            }
            trace.push_back({ "Action(" + to_string(prevState) + ", " + P.lhs + ") = r" + to_string(prodIndex),
                              to_string(pointer), lookahead, join_stack(stack_symbols) });
            if (!gotoVal.empty()) {
                stack_symbols.push_back(gotoVal);
            } else break;
        } else {
            break;
        }
    }

    if (accepted) {
        string content;
        vector<string> traceHeader = { "Process", "LookAhead", "Symbol", "Stack" };
        content += table_to_string(trace, traceHeader);
        string compressed = compress_name(input_line);
        save_result_file(content, grammar_id, compressed);
        cout << "The string \"" << input_line << "\" is parsable! Saved in parsable_strings/" << grammar_id << "/" << compressed << ".txt\n";
    } else {
        cout << "The string \"" << input_line << "\" is not parsable!\n";
    }

    return 0;
}