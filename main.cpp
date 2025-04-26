#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <queue>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <math.h>
#include "DTree.h"

// Oh yeah. We're totally doing this.
#include <omp.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Like strtok from c stdlib, but for std::string
static std::vector<std::string> strtokstr(std::string s, char sep) {
    std::vector<std::string> ret;
    std::stringstream token;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] != sep) { token << s[i]; }
        else { ret.push_back(token.str()); token.str(""); }
    }
    ret.push_back(token.str());
    return ret;
}

static std::vector<DataFrame> loadFile(size_t label, std::filesystem::path path) {
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        std::cerr << "Could Not Read File" << std::endl;
    }
    std::vector<DataFrame> ret;
    std::string line;
    std::ifstream file(path);
    std::string value;

    while (file) {
        std::getline(file, line);
        if (line == "") { continue; }
        std::vector<std::string> tokens = strtokstr(line, ' ');
        DataFrame df;
        for (size_t i = 0; i < tokens.size(); i++) {
            std::string token = tokens[i];
            if (i == label) {
                df.label = token;
            } else if (token.find(".") != std::string::npos) {
                df.fields.push_back(strtof(token.c_str(), nullptr));
            }
            else {
                bool alpha = false;
                for (size_t iter = token.length(); iter < token.length() && !alpha; iter++) { alpha |= static_cast<bool>(isalpha(token[iter])); }
                if (alpha) { df.label = token; }
                else { df.fields.push_back(strtol(token.c_str(), nullptr, 10)); }
            }
        }
        ret.push_back(df);
    }
    file.close();
    return ret;
}

static std::string DTreeClassify(const DTree& tree, const DataFrame* df) {  
   if (std::shared_ptr<DTree::DecisionNode> head = std::dynamic_pointer_cast<DTree::DecisionNode>(tree.head.lock())) {  
       std::shared_ptr<DTree::Node> cur = head;
       while (cur.get()->left_child.expired() || cur.get()->right_child.expired()) {
           if (std::dynamic_pointer_cast<DTree::DecisionNode>(cur).get()->decide(df)) {
               cur = cur.get()->left_child.lock();
           }
           else {
               cur = cur.get()->right_child.lock();
           }
       }
       return std::dynamic_pointer_cast<DTree::ValueNode>(cur).get()->modeLabel();
   } else {  
       return "Tree empty, or no decision nodes present in tree.";  
   }  
   return "How did you even get here?";
}

static double entropy(std::vector<const DataFrame*> data) {
    std::map<std::string, int> set_frequencies;
    for (const DataFrame* df : data) {
        if (set_frequencies.find(df->label) != set_frequencies.end()) {
            set_frequencies[df->label]++;
        }
        else {
            set_frequencies[df->label] = 0;
        }
    }
    double entropy = 0;
    for (std::pair<std::string, int> cat : set_frequencies) {
        double p = static_cast<double>(cat.second) / static_cast<double>(data.size());
        entropy += p * log2(p);
    }
    return -entropy;
}

static double giniImpurity(std::vector<const DataFrame*> data) {
    std::map<std::string, int> set_frequencies;
    for (const DataFrame* df : data) {
        if (set_frequencies.find(df->label) != set_frequencies.end()) {
            set_frequencies[df->label]++;
        }
        else {
            set_frequencies[df->label] = 0;
        }
    }
    double gini_impurity = 0;
    for (std::pair<std::string, int> label : set_frequencies) {
        double p = static_cast<double>(label.second) / static_cast<double>(data.size());
        gini_impurity += p * p;
    }
    return 1 - gini_impurity;
}

static double evaluateSplit(
    std::shared_ptr<DTree::ValueNode> toSplit, 
    DTree::DecisionNode::comparisonType comparisonType, 
    size_t comparisonField, 
    std::variant<int, float> compareAgainst, 
    double(*minimiseFunc)(std::vector<const DataFrame*>)
) {
    std::vector<const DataFrame*> data = toSplit->getEncompassedData();
    std::vector<const DataFrame*> data_a;
    std::vector<const DataFrame*> data_b;
    for (size_t i = 0; i < data.size(); i++) {
        switch (comparisonType)
        {
        case DTree::DecisionNode::equal:
            if (std::holds_alternative<int>(compareAgainst)) {
                if (std::get<int>(compareAgainst) == std::get<int>(data[i]->fields[comparisonField])) { 
                    data_a.push_back(data[i]); 
                } else { 
                    data_b.push_back(data[i]); 
                }
            }
            else if (std::holds_alternative<float>(compareAgainst)) {
                if (std::get<float>(compareAgainst) == std::get<float>(data[i]->fields[comparisonField])) { 
                    data_a.push_back(data[i]);
                } else { 
                    data_b.push_back(data[i]); 
                }
            }
            break;
        case DTree::DecisionNode::lessThan:
            if (std::holds_alternative<int>(compareAgainst)) {
                if (std::get<int>(data[i]->fields[comparisonField]) < std::get<int>(compareAgainst)) {
                    data_a.push_back(data[i]); 
                } else { 
                    data_b.push_back(data[i]); 
                }
            }
            else if (std::holds_alternative<float>(compareAgainst)) {
                if (std::get<float>(data[i]->fields[comparisonField]) < std::get<float>(compareAgainst)) {
                    data_a.push_back(data[i]); 
                } else { 
                    data_b.push_back(data[i]); 
                }
            }
            break;
        case DTree::DecisionNode::greaterThan:
            if (std::holds_alternative<int>(compareAgainst)) {
                if (std::get<int>(data[i]->fields[comparisonField]) > std::get<int>(compareAgainst)) {
                    data_a.push_back(data[i]); 
                } else { 
                    data_b.push_back(data[i]);
                }
            }
            else if (std::holds_alternative<float>(compareAgainst)) {
                if (std::get<float>(data[i]->fields[comparisonField]) > std::get<float>(compareAgainst)) {
                    data_a.push_back(data[i]);
                } else { 
                    data_b.push_back(data[i]); 
                }
            }
            break;
        default:
            break;
        }   
    }
    
    double loss_a = minimiseFunc(data_a);
    double loss_b = minimiseFunc(data_b);
    double fraction_a = static_cast<double>(data_a.size()) / static_cast<double>(data.size());
    double fraction_b = static_cast<double>(data_b.size()) / static_cast<double>(data.size());
    
    return minimiseFunc(data) - (fraction_a * loss_a + fraction_b * loss_b);
}

struct SplitDetails {
    double purity;
    struct { 
        DTree::DecisionNode::comparisonType type;
        size_t df_field;
        std::variant<int, float> constant;
    } comparison;
    bool operator<(const SplitDetails& other) const {
        return this->purity < other.purity;
    }
};

struct NodeSplitDetails {
    struct SplitDetails first;
    std::weak_ptr<DTree::ValueNode> second;
    bool operator<(const NodeSplitDetails& other) const {
        return this->first.purity < other.first.purity;
    }
};

static std::ostream& operator<<(std::ostream& os, const SplitDetails& sd) {
    os << "{purity: " << sd.purity << " split: [" << sd.comparison.df_field << "]";
    switch (sd.comparison.type) {
    case DTree::DecisionNode::comparisonType::equal: os << " = "; break;
    case DTree::DecisionNode::comparisonType::greaterThan: os << " > "; break;
    case DTree::DecisionNode::comparisonType::lessThan: os << " < "; break;
    }
    std::visit([&](auto&& arg) { os << arg; }, sd.comparison.constant);
    os << "}";
    return os;
}


// This function, based on some preliminary profiling runs 2.3x faster due to utilising OpenMP
// to multithread the main for loop. And it was so much easier to make work than python 
// multithreading! Yipee!
static std::priority_queue<struct SplitDetails> findBestSplits(
    std::shared_ptr<DTree::ValueNode> src,
    double(*evaluationFunction)(std::vector<const DataFrame*>),
    unsigned int samples,
    bool continuousInts = false,
    bool useGreaterThan = false
) {
    std::vector<const DataFrame*> data = src->getEncompassedData();
    if (data.size() == 0) { throw "Cannot split node with no data"; }
    size_t df_fields = data[0]->fields.size();

    // Create thread-local priority queues to avoid race conditions
    std::vector<std::priority_queue<struct SplitDetails>> thread_queues(omp_get_max_threads());

    // Parallelize the loop over field indices
#pragma omp parallel for
    for (int field_index = 0; field_index < df_fields; field_index++) {
        // Get the thread-local queue for this thread
        int thread_id = omp_get_thread_num();
        std::priority_queue<struct SplitDetails>& local_queue = thread_queues[thread_id];

        std::vector<const DataFrame*> temp_copy = data;
        const DataFrame* min_element = nullptr;
        const DataFrame* max_element = nullptr;
        for (const DataFrame* elem : temp_copy) {
            if (min_element == nullptr || elem->fields[field_index] < min_element->fields[field_index]) { min_element = elem; }
            if (max_element == nullptr || elem->fields[field_index] > min_element->fields[field_index]) { max_element = elem; }
        }

        if (min_element == max_element) {
            local_queue.push({ 
                1.0, 
                { 
                    DTree::DecisionNode::comparisonType::equal, 
                    static_cast<size_t>(field_index), 
                    min_element->fields[field_index] 
                } 
            });
            continue;
        }

        if (!continuousInts && std::holds_alternative<int>(temp_copy[0]->fields[field_index])) {
            // Ignore the samples parameter - we'll just check all the value we know about.
            for (int j = std::get<int>(min_element->fields[field_index]); j <= std::get<int>(max_element->fields[field_index]); j++) {
                local_queue.push({ 
                    evaluateSplit(
                        src, 
                        DTree::DecisionNode::comparisonType::equal, 
                        static_cast<size_t>(field_index), 
                        j, 
                        evaluationFunction
                    ), 
                    { 
                        DTree::DecisionNode::comparisonType::equal,
                        static_cast<size_t>(field_index), 
                        j 
                    } 
                });
            }
        }
        else {
            if (std::holds_alternative<int>(temp_copy[0]->fields[field_index])) {
                // Try to find the right number of samples, but might not find that many
                int diff = std::get<int>(max_element->fields[field_index]) - std::get<int>(min_element->fields[field_index]);
                std::set<int> samples_set;
                for (unsigned int j = 0; j < samples; j++) {
                    int sample = static_cast<int>(roundf(std::get<int>(min_element->fields[field_index]) + diff * (static_cast<float>(j) / static_cast<float>(samples))));
                    samples_set.insert(sample);
                }
                if (useGreaterThan) {
                    for (int sample : samples_set) {
                        local_queue.push({ 
                            evaluateSplit(
                                src, 
                                DTree::DecisionNode::comparisonType::greaterThan,
                                static_cast<size_t>(field_index),
                                sample,
                                evaluationFunction),
                            { 
                                DTree::DecisionNode::comparisonType::greaterThan,
                                static_cast<size_t>(field_index),
                                sample
                            }
                        });
                    }
                }
                else {
                    for (int sample : samples_set) {
                        local_queue.push({ 
                            evaluateSplit(
                                src, 
                                DTree::DecisionNode::comparisonType::lessThan,
                                static_cast<size_t>(field_index),
                                sample,
                                evaluationFunction
                            ), 
                            { 
                                DTree::DecisionNode::comparisonType::lessThan,
                                static_cast<size_t>(field_index),
                                sample 
                            } 
                        });
                    }
                }
            }
            else if (std::holds_alternative<float>(temp_copy[0]->fields[field_index])) {
                float diff = std::get<float>(max_element->fields[field_index]) - std::get<float>(min_element->fields[field_index]);
                if (useGreaterThan) {
                    for (unsigned int j = 0; j < samples; j++) {
                        float sample = std::get<float>(min_element->fields[field_index]) + diff * (static_cast<float>(j) / static_cast<float>(samples));
                        local_queue.push({ 
                            evaluateSplit(
                                src, 
                                DTree::DecisionNode::comparisonType::greaterThan, 
                                static_cast<size_t>(field_index), 
                                sample,
                                evaluationFunction
                            ), 
                            { 
                                DTree::DecisionNode::comparisonType::greaterThan, 
                                static_cast<size_t>(field_index), 
                                sample 
                            } });
                    }
                }
                else {
                    for (unsigned int j = 0; j < samples; j++) {
                        float sample = std::get<float>(min_element->fields[field_index]) + diff * (static_cast<float>(j) / static_cast<float>(samples));
                        local_queue.push({ 
                            evaluateSplit(
                                src, 
                                DTree::DecisionNode::comparisonType::lessThan,
                                static_cast<size_t>(field_index),
                                sample, 
                                evaluationFunction
                            ), 
                            { 
                                DTree::DecisionNode::comparisonType::lessThan, 
                                static_cast<size_t>(field_index), 
                                sample 
                            } 
                        });
                    }
                }
            }
        }
    }

    // Merge all thread-local queues into the final result queue
    std::priority_queue<struct SplitDetails> ret;
    for (auto& local_queue : thread_queues) {
        while (!local_queue.empty()) {
            ret.push(local_queue.top());
            local_queue.pop();
        }
    }

    return ret;
}


enum LimitingFactor { depth, decisions, leaves };
static DTree trainDTree(
    std::vector<DataFrame> data, 
    double(*evaluationFunction)(std::vector<const DataFrame*>),
    unsigned int samples,
    LimitingFactor limiting_factor,
    unsigned int limit,
    bool continuousInts = false,
    bool useGreaterThan = false
) {
    DTree tree(data);
    if (limiting_factor == decisions) {
        std::priority_queue<NodeSplitDetails> splits;
        for (int i = 0; i < limit; i++) {
            for (auto leaf : tree.get_leaves()) {
                if (!leaf.expired()) {
                    auto best_splits = findBestSplits(leaf.lock(), evaluationFunction, samples, continuousInts, useGreaterThan);
                    if (best_splits.size() == 0) {
                        continue;
                    }
                    splits.push({ best_splits.top(), leaf });
                }
                else {
                    std::cerr << "Attempted to access empty leaf" << std::endl;
                }
            }
            // get first split
            auto split = splits.top(); 
            splits.pop();

            // reject splits until we have one on a value node that currently exists
            // also reject splits that produce a value node with nothing in it.
            while (true) {
                if (split.second.expired()) { 
                    split = splits.top();
                    splits.pop();
                    continue;
                }
                auto preemptive_split = DTree::DecisionNode::evaluate(
                    split.first.comparison.df_field,
                    split.first.comparison.type,
                    split.first.comparison.constant,
                    split.second.lock()->getEncompassedData()
                );
                if (preemptive_split.first.size() == 0 || preemptive_split.second.size() == 0) { 
                    split = splits.top();
                    splits.pop();
                    continue;
                }
                
            }

            tree.split_leaf(split.second.lock(), split.first.comparison.df_field, split.first.comparison.type, split.first.comparison.constant);
        }
        return tree;
    } 
    else if (limiting_factor == depth) {
        std::priority_queue<NodeSplitDetails> splits;
        while (tree.max_depth() <= limit) {
            for (auto leaf : tree.get_leaves()) {
                if (leaf.lock()->getEncompassedData().size() < 2) { std::cout << "Prevented splitting small leaf" << std::endl; }
                else {
                    auto best_splits = findBestSplits(leaf.lock(), evaluationFunction, samples, continuousInts, useGreaterThan);
                    std::cout << "Found " << best_splits.size() << "splits..." << std::endl;
                    splits.push({ best_splits.top(), leaf });
                }
            }
            if (splits.size() == 0) { std::cout << "No more nodes I can split" << std::endl;  break; }
            auto split = splits.top(); splits.pop();
            while ((split.second.expired() || split.second.lock()->depth() >= limit) && splits.size() > 0) { 
                split = splits.top(); splits.pop(); 
            }
            if (!(split.second.expired() || split.second.lock()->depth() >= limit)) {
                std::cout << "Split on field " << split.first.comparison.df_field << std::endl;
                tree.split_leaf(split.second.lock(), split.first.comparison.df_field, split.first.comparison.type, split.first.comparison.constant);
            }
            else {
                break;
            }
        }
        return tree;
    } 
    else if (limiting_factor == leaves) {
        std::priority_queue<NodeSplitDetails> splits;
        while (tree.get_leaves().size() <= limit + 1) {
            for (auto leaf : tree.get_leaves()) {
                auto best_splits = findBestSplits(leaf.lock(), evaluationFunction, samples, continuousInts, useGreaterThan);
                splits.push({ best_splits.top(), leaf });
            }
            auto split = splits.top(); splits.pop();
            while (split.second.expired()) { split = splits.top(); splits.pop(); }
            tree.split_leaf(split.second.lock(), split.first.comparison.df_field, split.first.comparison.type, split.first.comparison.constant);
        }
        return tree;
    } 
    else {
        return tree;
    }
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
        ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    std::locale::global(std::locale("en_US.UTF-8"));
    std::cout.imbue(std::locale());
#endif

    std::vector<DataFrame> dfs = loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\training.dat");
    std::cout << "Loaded " << dfs.size() << " DataFrames from file." << std::endl;
    
    DTree tree = trainDTree(dfs, entropy, 2, depth, 3, false, false);

    std::cout << tree.to_string();
}

