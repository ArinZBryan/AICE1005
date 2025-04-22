#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <cctype>
#include <math.h>
#include "DTree.h"

// Like strtok from c stdlib, but for std::strings
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

double evaluateSplit(DTree::ValueNode toSplit, DTree::DecisionNode::comparisonType comparisonType, size_t comparisonField, std::variant<int, float> compareAgainst, double(*minimiseFunc)(std::vector<DataFrame>)) {
    const std::vector<DataFrame>& data = toSplit.getEncompassedData();
    std::vector<DataFrame> data_a;
    std::vector<DataFrame> data_b;
    for (size_t i = 0; i < data.size(); i++) {
        switch (comparisonType)
        {
        case DTree::DecisionNode::equal:
            if (std::get<int>(compareAgainst) == std::get<int>(data[i].fields[comparisonField])) { data_a.push_back(data[i]); }
            break;
        case DTree::DecisionNode::lessThan:
            if (std::get<float>(compareAgainst) < std::get<float>(data[i].fields[comparisonField])) { data_a.push_back(data[i]); }
            break;
        case DTree::DecisionNode::greaterThan:
            if (std::get<float>(compareAgainst) > std::get<float>(data[i].fields[comparisonField])) { data_a.push_back(data[i]); }
            break;
        default:
            break;
        }
    }
    return minimiseFunc(data) - (data_a.size() / data.size() * minimiseFunc(data_a) + data_b.size() / data.size() * minimiseFunc(data_b));
}

struct SplitDetails {
    double purity;
    struct { 
        DTree::DecisionNode::comparisonType type;
        size_t df_field;
        std::variant<int, float> constant;
    } comparison;
};

std::vector<struct SplitDetails> findBestSplits(DTree::ValueNode src, double(*evaluationFunction)(std::vector<DataFrame>), unsigned int samples, bool allowIntEquals) {
    const std::vector<DataFrame>& data = src.getEncompassedData();
    if (data.size() == 0) { return {}; }
    size_t df_fields = data[0].fields.size();
    for (size_t i = 0; i < df_fields; i++) {

    }
};

int main()
{
    std::cout << "Hello World!\n";
    std::vector<DataFrame> dfs = loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\training.dat");
    std::cout << "Loaded " << dfs.size() << " DataFrames from file." << std::endl;
    auto tree = DTree(dfs);
    std::cout << tree.to_string() << std::endl;
    std::cout << "Gini Impurity: " << giniImpurity(std::dynamic_pointer_cast<DTree::ValueNode>(tree.head.lock()).get()->getEncompassedData()) << std::endl;
    std::cout << "Entropy: " << entropy(std::dynamic_pointer_cast<DTree::ValueNode>(tree.head.lock()).get()->getEncompassedData()) << std::endl;
}

