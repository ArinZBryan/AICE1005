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


// TODO: See if we can reduce the use of the copy constructor DataFrame::DataFrame(const DataFrame&).
//       It's still taking 10-20% of all runtime :( - I think most of its use seems to be in constructing
//       vectors, but I don't know if that's avoidable.
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
    DTree(std::vector<DataFrame> data_points);

    enum printStyle { percent, size, address, none };

    printStyle print_style;
    std::weak_ptr<Node> head;
private:
    std::set<std::shared_ptr<Node>> nodes;
    std::vector<DataFrame> data;
};
