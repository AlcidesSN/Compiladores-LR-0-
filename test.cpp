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


using namespace std;

/* 
Function that is responsible for changing the position of the ".", indicates the first term, it is done only once. 
*/
string insert_dot(const string& production) {
    string s = production;
    size_t pos = s.find("->");
    if (pos != string::npos) s.replace(pos, 2, "->.");
    return s;
}

/* 
A função 'vector_key' pega cada string do vetor e vai colando tudo em uma única string só, separando 
cada elemento com o marcador |#|. Se for o primeiro elemento, ela coloca direto; a partir do segundo, ela 
coloca |#| antes. 
*/

string vector_key(const vector<string>& v) {
    string out;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) out += "|#|";
        out += v[i];
    }
    return out;
}

/* A função 'closure' basicamente pega um item LR(0) e sai adicionando ao conjunto todas as produções 
que precisam aparecer quando, logo depois do ponto '.', aparece um nao-terminal. Ela começa colocando 
o item inicial no vetor e vai analisando cada um deles. Quando encontra o '.', olha o símbolo que 
vem depois, se for o mesmo símbolo que inicia alguma produção, ela cria uma nova versão dessa produção 
com o '.' no começo e adiciona ao conjunto, sem repetir itens. Esse processo continua até não aparecer 
mais nada novo*/

vector<string> closure(const string& item, const vector<string>& productions) {
    vector<string> result;
    result.push_back(item);
    for (size_t idx = 0; idx < result.size(); ++idx) {
        string current = result[idx];
        size_t dotPos = current.find('.');
        if (dotPos != string::npos && dotPos + 1 < current.size()) {
            char nextSymbol = current[dotPos + 1];
            for (const string& prod : productions) {
                if (!prod.empty() && prod[0] == nextSymbol) {
                    string withDot = insert_dot(prod);
                    if (find(result.begin(), result.end(), withDot) == result.end()) {
                        result.push_back(withDot);
                    }
                }
            }
        }
    }
    return result;
}
/*
 Is designed to change the position of the dot (.) in a string 
 representation of a grammar item (e.g., "S -> .aB" becomes "S -> a.B" after swapping).
*/
string swap_positions(const string& sequence, size_t position) {
    if (position + 1 >= sequence.size()) return sequence;
    string s = sequence;
    swap(s[position], s[position + 1]);
    return s;
}

vector<string> goto_state(const string& item, const vector<string>& productions) {
    size_t dotIndex = item.find('.');
    if (dotIndex == string::npos) return { item };
    if (dotIndex == item.size() - 1) return { item };
    string moved = swap_positions(item, dotIndex);
    if (moved.find('.') != string::npos && moved.find('.') != moved.size() - 1) {
        return closure(moved, productions);
    }
    return { moved };
}

/* 
Here is made the separation of all the terminal states, it stores each one inside a list.
For each production, it finds the "->" and isolates what it produces, after that it goes 
throught each of the characters, it skips certain characters (like "." and " ") and if it's
not a upper case letter, the character is added to the "set<char>", and it add's a "$" 
to use it like a "end-of-production-marker".
At the end, the list of characters is returned to a vector so it can be sorted out in the next part.
*/
set<char> get_terminals(const vector<string>& productions) {
    set<char> terms;
    for (const string& p : productions) {
        size_t pos = p.find("->");
        if (pos == string::npos) continue;
        string right = p.substr(pos + 2);
        for (char c : right) {
            if (c == '.' || c == ' ' || c == '\t') continue;
            if (!(c >= 'A' && c <= 'Z')) terms.insert(c);
        }
    }
    terms.insert('$');
    return terms;
}

/*
Here it process each production rule just like it is done with get_terminals, except, now, each
character that is an upper case letter (A-Z) is considered a non terminal and is added to the "set<char>".
It returns the set without any additional markers ($) cause it doesn't indicate a 'end' but rather a follow.
At the end, the list of characters is returned to a vector so it can be sorted out in the next part.
*/
set<char> get_non_terminals(const vector<string>& productions) {
    set<char> nonterms;
    for (const string& p : productions) {
        size_t pos = p.find("->");
        if (pos == string::npos) continue;
        string right = p.substr(pos + 2);
        for (char c : right) {
            if (c >= 'A' && c <= 'Z') nonterms.insert(c);
        }
    }
    return nonterms;
}

vector<pair<string, vector<string>>> filter_by_state(const unordered_map<string, vector<string>>& graph, int state) {
    vector<pair<string, vector<string>>> out;
    for (const auto& kv : graph) {
        const string& key = kv.first;
        stringstream ss(key);
        int s;
        ss >> s;
        if (s == state) out.push_back(kv);
    }
    return out;
}

string compress_name(const string& name) {
    unordered_map<char, int> freq;
    for (char c : name) freq[c]++;
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

//essa primeira parte ela vai receber uma tabela, que no caso acho que é a vector<vector<string>> table,
//ela recebe também um cabeçalho que é o header e depois ele vai contruir um tipo de representação de tabela, dai no final retorna essa tabela como string usando stringstream ss; e return ss.str();.
//isso é uma visão mais geral que eu entendi, é basicamente uma construção de uma tabela onde a função vector<vector<string>> table e vector<string> header criam essa tabela e usam | e +/- para ficar alinhando as colunas para a direita, e por fim retorna tudo como string usando as funções: stringstream ss; e return ss.str()
/* */
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
    cout << "LR (0) Parsing\n\n";

    vector<string> productions;
    vector<vector<string>> item_sets;
    vector<vector<string>> completed_sets;

/*
Nessa função, é pedido para o usuário informar o id para que a função possa chamar o grammar/<id>.txt correspondente. 
E depois, lê linha a linha cada produção não-vazia para o vetor productions (falha com cerr e return 1 se o arquivo não abrir); 
Em seguida insere no índice zero a produção aumentada "S'->.S" — necessária para o ponto de partida do LR(0) — e constrói um unordered_map<string,int
*/
    int grammar_id;
    cout << "Enter grammar number: ";
    cin >> grammar_id;
    cout << "\n";

    string grammar_path = "grammar/" + to_string(grammar_id) + ".txt";
    ifstream gfile(grammar_path);
    if (!gfile.is_open()) {
        cerr << "Cannot open grammar file: " << grammar_path << endl;
        return 1;
    }
    string line;
    while (getline(gfile, line)) {
        if (!line.empty()) {
            productions.push_back(line);
        }
    }
    gfile.close();

    productions.insert(productions.begin(), "S'->.S");

    unordered_map<string, int> production_numbers;
    for (size_t i = 1; i < productions.size(); ++i) {
        production_numbers[productions[i]] = static_cast<int>(i);
    }

    vector<string> initial = closure(string("S'->.S"), productions);
    item_sets.push_back(initial);

    unordered_map<string, int> state_ids;
    unordered_map<string, vector<string>> dfa_transitions;
    int current_state_id = 0;

    while (!item_sets.empty()) {
        vector<string> current_items = item_sets.front();
        item_sets.erase(item_sets.begin());
        completed_sets.push_back(current_items);
        string keyCurrent = vector_key(current_items);
        state_ids[keyCurrent] = current_state_id;
        current_state_id++;

        for (const string& it : current_items) {
            vector<string> next_items = goto_state(it, productions);
            if (find(item_sets.begin(), item_sets.end(), next_items) == item_sets.end() && find(completed_sets.begin(), completed_sets.end(), next_items) == completed_sets.end()) {
                if (next_items != current_items) item_sets.push_back(next_items);
            }
            string mapKey = to_string(state_ids[keyCurrent]) + " " + it;
            dfa_transitions[mapKey] = next_items;
        }
    }

    for (size_t i = 0; i < completed_sets.size(); ++i) {
        for (size_t j = 0; j < completed_sets[i].size(); ++j) {
            vector<string> g = goto_state(completed_sets[i][j], productions);
            if (find(completed_sets.begin(), completed_sets.end(), g) == completed_sets.end()) {
                if (completed_sets[i][j].find('.') != string::npos && completed_sets[i][j].find('.') != completed_sets[i][j].size() - 1) {
                    completed_sets.push_back(g);
                }
            }
        }
    }

    cout << "---------------------------------------------------------------\n";
    cout << "Total States: " << completed_sets.size() << "\n";
    for (size_t i = 0; i < completed_sets.size(); ++i) {
        cout << i << " : [";
        for (size_t j = 0; j < completed_sets[i].size(); ++j) {
            if (j) cout << ", ";
            cout << completed_sets[i][j];
        }
        cout << "]\n";
    }
    cout << "---------------------------------------------------------------\n";

    map<int, map<char, int>> dfa;
    for (size_t i = 0; i < completed_sets.size(); ++i) {
        auto trans = filter_by_state(dfa_transitions, static_cast<int>(i));
        map<char, int> symbol_to_state;
        for (const auto& kv : trans) {
            string key = kv.first;
            string item = key.substr(key.find(' ') + 1);
            size_t arrow = item.find("->");
            if (arrow == string::npos) continue;
            string right = item.substr(arrow + 2);
            size_t dot = right.find('.');
            if (dot == string::npos || dot + 1 >= right.size()) continue;
            char nextSymbol = right[dot + 1];
            string nextItemsKey = vector_key(kv.second);
            if (state_ids.find(nextItemsKey) != state_ids.end()) {
                symbol_to_state[nextSymbol] = state_ids[nextItemsKey];
            }
        }
        if (!symbol_to_state.empty()) {
            dfa[static_cast<int>(i)] = symbol_to_state;
        }
    }

    set<char> terminals_set = get_terminals(productions);
    set<char> nonterm_set = get_non_terminals(productions);
    vector<char> terminals(terminals_set.begin(), terminals_set.end());
    vector<char> nonterms(nonterm_set.begin(), nonterm_set.end());
    sort(terminals.begin(), terminals.end());
    sort(nonterms.begin(), nonterms.end());

    vector<vector<string>> table_rows;
    vector<string> header;
    header.push_back("");
    for (char t : terminals) header.push_back(string(1, t));
    for (char nt : nonterms) header.push_back(string(1, nt));

    unordered_map<int, unordered_map<char, string>> parsing_table_dict;

    for (size_t i = 0; i < completed_sets.size(); ++i) {
        vector<string> row;
        row.push_back(to_string(static_cast<int>(i)));
        row.resize(1 + terminals.size() + nonterms.size(), "");
        unordered_map<char, string> actions;

        if (dfa.find(static_cast<int>(i)) != dfa.end()) {
            for (const auto& kv : dfa[static_cast<int>(i)]) {
                char sym = kv.first;
                int st = kv.second;
                if (sym >= 'A' && sym <= 'Z') {
                    size_t idx = 1 + terminals.size() + (find(nonterms.begin(), nonterms.end(), sym) - nonterms.begin());
                    if (idx < row.size()) row[idx] = to_string(st);
                    actions[sym] = to_string(st);
                } else {
                    size_t idx = 1 + (find(terminals.begin(), terminals.end(), sym) - terminals.begin());
                    if (idx < row.size()) row[idx] = "S" + to_string(st);
                    actions[sym] = "S" + to_string(st);
                }
            }
        }

        for (const string& it : completed_sets[i]) {
            size_t dotPos = it.find('.');
            if (dotPos != string::npos && dotPos == it.size() - 1) {
                string prodNoDot = it;
                prodNoDot.erase(prodNoDot.find('.'), 1);
                if (prodNoDot == "S'->S") {
                    actions['$'] = "Accept";
                    size_t idx = 1 + (find(terminals.begin(), terminals.end(), '$') - terminals.begin());
                    if (idx < row.size()) row[idx] = "Accept";
                } else {
                    auto itnum = production_numbers.find(prodNoDot);
                    if (itnum != production_numbers.end()) {
                        int prodIndex = itnum->second;
                        for (size_t k = 0; k < terminals.size(); ++k) {
                            row[1 + k] = "r" + to_string(prodIndex);
                            actions[terminals[k]] = "r" + to_string(prodIndex);
                        }
                    }
                }
            }
        }

        parsing_table_dict[static_cast<int>(i)] = actions;
        table_rows.push_back(row);
    }

    vector<string> header_for_print;
    header_for_print.push_back("State");
    for (char t : terminals) header_for_print.push_back(string(1, t));
    for (char nt : nonterms) header_for_print.push_back(string(1, nt));
    cout << "\nParsing Table:\n\n";
    cout << table_to_string(table_rows, header_for_print) << "\n";

    cout << "\nEnter the string to be parsed: ";
    string input_string;
    cin >> ws;
    getline(cin, input_string);
    input_string += '$';

    vector<string> stack;
    stack.push_back("0");
    int pointer = 0;
    vector<vector<string>> trace;
    bool accepted = false;

    while (true) {
        if (stack.empty()) break;
        int state = stoi(stack.back());
        char lookahead = pointer < (int)input_string.size() ? input_string[pointer] : '\0';

        string action;
        if (parsing_table_dict.count(state) && parsing_table_dict[state].count(lookahead)) {
            action = parsing_table_dict[state][lookahead];
        } else {
            break;
        }

        if (action == "Accept") {
            trace.push_back({ "Action(" + to_string(state) + ", " + string(1, lookahead) + ") = Accept",
                              to_string(pointer), string(1, lookahead), join_stack(stack) });
            accepted = true;
            break;
        } else if (!action.empty() && action[0] == 'S') {
            int nextState = stoi(action.substr(1));
            trace.push_back({ "Action(" + to_string(state) + ", " + string(1, lookahead) + ") = " + action,
                              to_string(pointer), string(1, lookahead), join_stack(stack) });
            string sym(1, lookahead);
            stack.push_back(sym);
            stack.push_back(to_string(nextState));
            pointer++;
        } else if (!action.empty() && action[0] == 'r') {
            int prodIndex = stoi(action.substr(1));
            string prodToReduce;
            for (const auto& kv : production_numbers) {
                if (kv.second == prodIndex) { prodToReduce = kv.first; break; }
            }
            if (prodToReduce.empty()) break;
            string rhs = prodToReduce.substr(prodToReduce.find("->") + 2);
            int lengthToPop = 2 * static_cast<int>(rhs.size());
            for (int k = 0; k < lengthToPop; ++k) {
                if (!stack.empty()) stack.pop_back();
            }
            char lhs = prodToReduce[0];
            stack.push_back(string(1, lhs));
            int prevState = stoi(stack[stack.size() - 2]);
            string gotoVal = "";
            if (parsing_table_dict.count(prevState) && parsing_table_dict[prevState].count(lhs)) {
                gotoVal = parsing_table_dict[prevState][lhs];
            } else {
                break;
            }
            trace.push_back({ "Action(" + to_string(prevState) + ", " + string(1, lhs) + ") = r" + to_string(prodIndex),
                              to_string(pointer), string(1, lookahead), join_stack(stack) });
            if (!gotoVal.empty()) {
                stack.push_back(gotoVal);
            } else break;
        } else {
            break;
        }
    }

    /* Final validation checking each of the steps of the process, then makes the final say if it is or not parsable.*/
    if (accepted) {
        string content;
        vector<string> traceHeader = { "Process", "LookAhead", "Symbol", "Stack" };
        content += table_to_string(trace, traceHeader);
        string compressed = compress_name(input_string.substr(0, input_string.size() - 1));
        save_result_file(content, grammar_id, compressed);
        cout << "A string " << input_string.substr(0, input_string.size() - 1) << " e parsiavel(Aceita)! Salvo em parsable_strings/" << grammar_id << "/" << compressed << ".txt\n";
    } else {
        cout << "A string " << input_string.substr(0, input_string.size() - 1) << " Nao e parsiavel(Rejeitada)!\n";
    }

    return 0;
}

        
