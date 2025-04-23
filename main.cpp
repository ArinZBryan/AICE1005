#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <functional>
#include <math.h>
#include "DTree.h"


// Like strtok from c stdlib, but for std::string
std::vector<std::string> strtokstr(std::string s, char sep) {
    std::vector<std::string> ret;
    std::stringstream token;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] != sep) { token << s[i]; }
        else { ret.push_back(token.str()); token.str(""); }
    }
    ret.push_back(token.str());
    return ret;
}

std::vector<DataFrame> loadFile(size_t label, std::filesystem::path path) {
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
            } else if (token.contains(".")) {
                df.fields.push_back(strtof(token.c_str(), nullptr));
            }
            else {
                bool alpha = false;
                for (size_t iter = token.length(); iter < token.length() && !alpha; iter++) { alpha |= isalpha(token[iter]); }
                if (alpha) { df.label = token; }
                else { df.fields.push_back(strtol(token.c_str(), nullptr, 10)); }
            }
        }
        ret.push_back(df);
    }
    file.close();
    return ret;
}

std::string DTreeClassify(const DTree& tree, DataFrame df) {  
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

double entropy(std::vector<DataFrame> data) {
    std::map<std::string, int> set_frequencies;
    for (DataFrame df : data) {
        if (set_frequencies.contains(df.label)) {
            set_frequencies[df.label]++;
        }
        else {
            set_frequencies[df.label] = 0;
        }
    }
    double entropy = 0;
    for (std::pair<std::string, int> cat : set_frequencies) {
        double p = static_cast<double>(cat.second) / static_cast<double>(data.size());
        entropy += p * log2(p);
    }
    return -entropy;
}

double giniImpurity(std::vector<DataFrame> data) {
    std::map<std::string, int> set_frequencies;
    for (DataFrame df : data) {
        if (set_frequencies.contains(df.label)) {
            set_frequencies[df.label]++;
        }
        else {
            set_frequencies[df.label] = 0;
        }
    }
    double gini_impurity = 0;
    for (std::pair<std::string, int> label : set_frequencies) {
        double p = static_cast<double>(label.second) / static_cast<double>(data.size());
        gini_impurity += p * p;
    }
    return 1 - gini_impurity;
}

double evaluateSplit(std::shared_ptr<DTree::ValueNode> toSplit, DTree::DecisionNode::comparisonType comparisonType, size_t comparisonField, std::variant<int, float> compareAgainst, double(*minimiseFunc)(std::vector<DataFrame>)) {
    const std::vector<DataFrame>& data = toSplit->getEncompassedData();
    std::vector<DataFrame> data_a;
    std::vector<DataFrame> data_b;
    for (size_t i = 0; i < data.size(); i++) {
        switch (comparisonType)
        {
        case DTree::DecisionNode::equal:
            if (std::holds_alternative<int>(compareAgainst)) {
                if (std::get<int>(compareAgainst) == std::get<int>(data[i].fields[comparisonField])) { 
                    data_a.push_back(data[i]); 
                } else { 
                    data_b.push_back(data[i]); 
                }
            }
            else if (std::holds_alternative<float>(compareAgainst)) {
                if (std::get<float>(compareAgainst) == std::get<float>(data[i].fields[comparisonField])) { 
                    data_a.push_back(data[i]);
                } else { 
                    data_b.push_back(data[i]); 
                }
            }
            break;
        case DTree::DecisionNode::lessThan:
            if (std::holds_alternative<int>(compareAgainst)) {
                if (std::get<int>(data[i].fields[comparisonField]) < std::get<int>(compareAgainst)) {
                    data_a.push_back(data[i]); 
                } else { 
                    data_b.push_back(data[i]); 
                }
            }
            else if (std::holds_alternative<float>(compareAgainst)) {
                if (std::get<float>(data[i].fields[comparisonField]) < std::get<float>(compareAgainst)) {
                    data_a.push_back(data[i]); 
                } else { 
                    data_b.push_back(data[i]); 
                }
            }
            break;
        case DTree::DecisionNode::greaterThan:
            if (std::holds_alternative<int>(compareAgainst)) {
                if (std::get<int>(data[i].fields[comparisonField]) > std::get<int>(compareAgainst)) {
                    data_a.push_back(data[i]); 
                } else { 
                    data_b.push_back(data[i]);
                }
            }
            else if (std::holds_alternative<float>(compareAgainst)) {
                if (std::get<float>(data[i].fields[comparisonField]) > std::get<float>(compareAgainst)) {
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

std::ostream& operator<<(std::ostream& os, const SplitDetails& sd) {
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

std::vector<struct SplitDetails> findBestSplits(
    std::shared_ptr<DTree::ValueNode> src, 
    double(*evaluationFunction)(std::vector<DataFrame>), 
    unsigned int samples, 
	bool continuousInts = false,
    bool useGreaterThan = false
) {
    const std::vector<DataFrame>& data = src->getEncompassedData();
    if (data.size() == 0) { return {}; }
    size_t df_fields = data[0].fields.size();
	std::vector<struct SplitDetails> ret;
    // Consider vectorising this with openmp (Configuration Options > C/C++ > Language > OpenMP Support)
    for (size_t field_index = 0; field_index < df_fields; field_index++) {
        // Partial application provided by the standard library? Functional C++? How many souls had to be
        // sacrificed to the C++ gods for me to have this?
        std::vector<DataFrame> temp_copy = data;
		auto comparator = std::bind(&DataFrame::lt, std::placeholders::_1, std::placeholders::_2, field_index);
		auto min_max_elements = std::minmax_element(temp_copy.begin(), temp_copy.end(), comparator);
		if (min_max_elements.first == min_max_elements.second) { 
			ret.push_back({ 1.0, { DTree::DecisionNode::comparisonType::equal, field_index, min_max_elements.first->fields[field_index] } });
			continue;
        }
        if (!continuousInts && std::holds_alternative<int>(temp_copy[0].fields[field_index])) {
            // Ignore the samples parameter - we'll just check all the value we know about.
			for (int j = std::get<int>(min_max_elements.first->fields[field_index]); j <= std::get<int>(min_max_elements.second->fields[field_index]); j++) {
			    ret.push_back({ evaluateSplit(src, DTree::DecisionNode::comparisonType::equal, field_index, j, evaluationFunction), { DTree::DecisionNode::comparisonType::equal, field_index, j } });
			}
        }
        else {
            if (std::holds_alternative<int>(temp_copy[0].fields[field_index])) {
                // Try to find the right number of samples, but might not find that many
                int diff = std::get<int>(min_max_elements.second->fields[field_index]) - std::get<int>(min_max_elements.first->fields[field_index]);
				std::set<int> samples_set;
                for (unsigned int j = 0; j < samples; j++) {
                    int sample = roundf(std::get<int>(min_max_elements.first->fields[field_index]) + diff * (static_cast<float>(j) / static_cast<float>(samples)));
                    samples_set.insert(sample);
                }
                if (useGreaterThan) {
                    for (int sample : samples_set) {
                        ret.push_back({ evaluateSplit(src, DTree::DecisionNode::comparisonType::greaterThan, field_index, sample, evaluationFunction), { DTree::DecisionNode::comparisonType::greaterThan, field_index, sample } });
                    }
                }
                else {
                    for (int sample : samples_set) {
                        ret.push_back({ evaluateSplit(src, DTree::DecisionNode::comparisonType::lessThan, field_index, sample, evaluationFunction), { DTree::DecisionNode::comparisonType::lessThan, field_index, sample } });
                    }
                }
                
            }
            else if (std::holds_alternative<float>(temp_copy[0].fields[1])) {
                float diff = std::get<float>(min_max_elements.second->fields[field_index]) - std::get<float>(min_max_elements.first->fields[field_index]);
                if (useGreaterThan) {
                    for (unsigned int j = 0; j < samples; j++) {
                        float sample = std::get<float>(min_max_elements.first->fields[field_index]) + diff * (static_cast<float>(j) / static_cast<float>(samples));
                        ret.push_back({ evaluateSplit(src, DTree::DecisionNode::comparisonType::greaterThan, field_index, sample, evaluationFunction), { DTree::DecisionNode::comparisonType::greaterThan, field_index, sample } });
                    }
                }
                else {
                    for (unsigned int j = 0; j < samples; j++) {
                        float sample = std::get<float>(min_max_elements.first->fields[field_index]) + diff * (static_cast<float>(j) / static_cast<float>(samples));
                        ret.push_back({ evaluateSplit(src, DTree::DecisionNode::comparisonType::lessThan, field_index, sample, evaluationFunction), { DTree::DecisionNode::comparisonType::lessThan, field_index, sample } });
                    }
                }
                
            }
        }
    }
    std::sort(ret.begin(), ret.end());
    return ret;
};

int main()
{
    std::vector<DataFrame> dfs = loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\training.dat");
    std::cout << "Loaded " << dfs.size() << " DataFrames from file." << std::endl;
    auto tree = DTree(dfs);
    
    std::cout << "Gini Impurity: " << giniImpurity(std::dynamic_pointer_cast<DTree::ValueNode>(tree.head.lock()).get()->getEncompassedData()) << std::endl;
    std::cout << "Entropy: " << entropy(std::dynamic_pointer_cast<DTree::ValueNode>(tree.head.lock()).get()->getEncompassedData()) << std::endl;
    
    std::vector<struct SplitDetails> splits = findBestSplits(
        std::dynamic_pointer_cast<DTree::ValueNode>(tree.head.lock()), 
        entropy, 
        2, 
        true, 
        true
    );

    std::cout << "Evaluated " << splits.size() << " split(s)" << std::endl;

    for (const struct SplitDetails& sd : splits) {
        std::cout << sd << std::endl;
    }
}

