#pragma once
#include <set>
#include <memory>
#include <optional>
#include <variant>
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <iostream>
#include <sstream>
#include <iterator>
#include <unordered_map>
#include <algorithm>
#include <cmath>

struct DataFrame {
    std::string label;
    std::vector<std::variant<int, float>> fields;
    bool lt(const DataFrame& other, size_t field) const;
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
    };

    class DecisionNode : public Node {
    public:
        enum comparisonType { equal, lessThan, greaterThan };
        size_t dataFrameField = -1;
        DecisionNode::comparisonType comparison;
        std::variant<int, float> compareAgainst;

        DecisionNode(DTree& tree);
        DecisionNode(std::weak_ptr<DTree::Node> parent);

        DecisionNode(DTree& tree, size_t dataFrameField, enum DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst);
        bool decide(DataFrame value);
        std::string toString();

    };

    class ValueNode : public Node {
    private:
        std::vector<DataFrame> encompassedData;
    public:

        const std::vector<DataFrame>& getEncompassedData();
        std::string modeLabel();
        std::map<std::string, int> proportionedLabel();


        ValueNode();
        ValueNode(DTree& tree);
        ValueNode(std::weak_ptr<DTree::DecisionNode> parent);
        ValueNode(DTree& tree, std::vector<DataFrame> data);
        ValueNode(std::weak_ptr<DTree::DecisionNode> parent, std::vector<DataFrame> data);

        std::string toString();
    };

    std::string to_string();

    void split_leaf(std::shared_ptr<ValueNode> leaf, size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst);

    DTree(std::vector<DataFrame> data_points);

    std::weak_ptr<Node> head;
private:
    std::set<std::shared_ptr<Node>> nodes;
    
};
