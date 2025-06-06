﻿#pragma once
#include <set>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>

#include <memory>
#include <optional>
#include <variant>
#include <string>

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

#include <iterator>
#include <algorithm>
#include <cmath>

#include <cctype>

#include "DebugLogger.hpp"

//#define USE_OPENMP
#ifdef USE_OPENMP
#include <omp.h>
#endif

struct DataFrame {
    std::string label;
    std::vector<std::variant<int, float>> fields;
    bool lt(const DataFrame& other, size_t field) const;
    DataFrame() = default;
    DataFrame(const DataFrame&) = default;
    DataFrame(DataFrame&&) = default;
};

std::ostream& operator<<(std::ostream& os, const DataFrame& df);

class DTree {
public:
    class Node {
    protected:
        Node(DTree& tree);
        Node(std::weak_ptr<DTree::Node> parent);

    public:
        DTree& tree;
        std::weak_ptr<DTree::Node> parent;
        std::weak_ptr<DTree::Node> left_child;
        std::weak_ptr<DTree::Node> right_child;

        virtual ~Node();
        virtual std::string toString();
        size_t depth();
    };

    class DecisionNode : public Node {
    public:
        enum comparisonType { equal, lessThan, greaterThan };
        size_t dataFrameField = -1;
        DecisionNode::comparisonType comparison;
        std::variant<int, float> compareAgainst;

        DecisionNode(DTree& tree);
        DecisionNode(std::weak_ptr<DTree::Node> parent);

        DecisionNode(DTree& tree, size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst);
        bool decide(const DataFrame* value) const;
        static bool decide(size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst, const DataFrame* value);
        std::pair<std::vector<const DataFrame*>, std::vector<const DataFrame*>> evaluate(const std::vector<const DataFrame*>& data) const;
        static std::pair<std::vector<const DataFrame*>, std::vector<const DataFrame*>> evaluate(size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst, const std::vector<const DataFrame*>& data);
        std::string toString();

    };

    class ValueNode : public Node {
    private:
        std::vector<const DataFrame*> encompassedData;
    public:

        std::vector<const DataFrame*> getEncompassedData() const;
        std::string modeLabel();
        std::map<std::string, int> proportionedLabel();


        ValueNode();
        ValueNode(DTree& tree);
        ValueNode(std::weak_ptr<DTree::DecisionNode> parent);
        ValueNode(DTree& tree, std::vector<const DataFrame*> data);
        ValueNode(std::weak_ptr<DTree::DecisionNode> parent, std::vector<const DataFrame*> data);

        std::string toString();
    };

    std::string to_string() const;
    void split_leaf(std::shared_ptr<ValueNode> leaf, size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst);
    size_t max_depth();
    std::vector<std::weak_ptr<DTree::ValueNode>> get_leaves();


    class Training {
    public:
        static std::vector<DataFrame> loadFile(size_t label, std::filesystem::path path);
        static std::vector<DataFrame> loadFileIntRange(size_t label, unsigned int no_ranges, std::filesystem::path path);
        static std::vector<DataFrame> loadFileFloatRange(size_t label, unsigned int no_ranges, std::filesystem::path path);
        class LossFunctions {
        public:
            static double entropy(std::vector<const DataFrame*> data);
            static double giniImpurity(std::vector<const DataFrame*> data);
        };
        static double evaluateSplit(
            std::shared_ptr<ValueNode> toSplit,
            DTree::DecisionNode::comparisonType comparisonType,
            size_t comparisonField,
            std::variant<int, float> compareAgainst,
            double(*minimiseFunc)(std::vector<const DataFrame*>)
        );
        struct SplitDetails {
            double purity;
            struct {
                DecisionNode::comparisonType type;
                size_t df_field;
                std::variant<int, float> constant;
            } comparison;
            bool operator<(const SplitDetails& other) const {
                return this->purity < other.purity;
            }
        };
        struct NodeSplitDetails {
            struct SplitDetails first;
            std::weak_ptr<ValueNode> second;
            bool operator<(const NodeSplitDetails& other) const {
                return this->first.purity < other.first.purity;
            }
        };
        static std::priority_queue<struct DTree::Training::SplitDetails> findBestSplits(
            std::shared_ptr<DTree::ValueNode> src,
            double(*evaluationFunction)(std::vector<const DataFrame*>),
            unsigned int samples,
            bool continuousInts,
            bool useGreaterThan
        );
        enum LimitingFactor { depth, decisions, leaves };
        static void train(
            DTree& tree,
            double(*evaluationFunction)(std::vector<const DataFrame*>),
            unsigned int samples,
            LimitingFactor limiting_factor,
            unsigned int limit,
            bool continuousInts = false,
            bool useGreaterThan = false
        );
    };

    void train(
        double(*evaluationFunction)(std::vector<const DataFrame*>),
        unsigned int samples,
        Training::LimitingFactor limiting_factor,
        unsigned int limit,
        bool continuousInts = false,
        bool useGreaterThan = false
    );

    std::string classify(const DataFrame*);
    std::vector<std::string> classify(const std::vector<const DataFrame*>&);
    bool test(const DataFrame*);
    double test(const std::vector<const DataFrame*>&);

    DTree(std::vector<DataFrame> data_points);
    DTree(std::vector<DataFrame> data_points, std::vector<size_t> fields);

    enum printStyle { percent, size, address, none };
    Training training;
    printStyle print_style;
    std::weak_ptr<Node> head;
    std::vector<size_t> fields;
protected:
    DTree();
    static std::vector<size_t> make_fields_arg(const std::vector<DataFrame>& data_points);
    std::set<std::shared_ptr<Node>> nodes;
private:
    std::vector<DataFrame> data;
};

// Same as a DTree, but it doesn't own the DataFrames it has. Primarily for use in conjunction
// with a DForest, as the DForest owns the data, and passes non-owining references instead of
// making possbily hundreds of copies of the data. Note that because DTree::data is private, 
// DTree_Weak::data_weak is provided instead.
class DTree_Weak : public DTree {
public:
    DTree_Weak(std::vector<const DataFrame*> data_points);
    DTree_Weak(std::vector<const DataFrame*> data_points, std::vector<size_t> fields);
protected:
    static std::vector<size_t> make_fields_arg(const std::vector<const DataFrame*>& data_points);
private:
    std::vector<const DataFrame*> data_weak;
};

std::ostream& operator<<(std::ostream& os, const DTree::Training::SplitDetails& sd);