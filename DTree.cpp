#include "DTree.h"

std::ostream& operator<<(std::ostream& os, const DataFrame& df) {
	os << df.label << ":";

	for (const std::variant<int, float>& field : df.fields) {
		if (std::holds_alternative<int>(field)) {
			os << ' ' << std::get<int>(field);
		}
		else if (std::holds_alternative<float>(field)) {
			os << ' ' << std::get<float>(field);
		}
	}

	return os;
}

DTree::Node::Node(DTree& tree) : tree(tree) {}
DTree::Node::Node(std::weak_ptr<DTree::Node> parent) : parent(parent), tree(parent.lock()->tree) {}
DTree::Node::~Node() = default;
std::string DTree::Node::toString() { return "Node"; };

DTree::DecisionNode::DecisionNode(DTree& tree) : Node(tree) {};
DTree::DecisionNode::DecisionNode(std::weak_ptr<DTree::Node> parent) : Node(parent) {};
DTree::DecisionNode::DecisionNode(DTree& tree, size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst)
: Node(tree), dataFrameField(dataFrameField), comparison(comparison), compareAgainst(compareAgainst) {}
bool DTree::DecisionNode::decide(DataFrame value) { 
	bool ret = false;
	if (this->comparison == equal) {
		return this->compareAgainst == value.fields[dataFrameField];
	}
	else if (this->comparison == lessThan) {
		return this->compareAgainst < value.fields[dataFrameField];
	}
	else if (this->comparison == greaterThan) {
		return this->compareAgainst > value.fields[dataFrameField];
	}
	return false;
}
std::string DTree::DecisionNode::toString() {
	std::stringstream stream;
	stream << "[" << dataFrameField << "] ";
	switch (comparison)
	{
	case DTree::DecisionNode::equal: stream << "= "; break;
	case DTree::DecisionNode::lessThan: stream << "< "; break;
	case DTree::DecisionNode::greaterThan: stream << "> "; break;
	default: stream << "? "; break;
	}
	std::visit([&](auto&& arg) { stream << arg; }, compareAgainst);
	return stream.str();
}

DTree::ValueNode::ValueNode() : Node(tree) {}
DTree::ValueNode::ValueNode(DTree& tree) : Node(tree) {}
DTree::ValueNode::ValueNode(std::weak_ptr<DTree::DecisionNode> parent) : Node(parent) {}
DTree::ValueNode::ValueNode(DTree& tree, std::vector<DataFrame> data) : Node(tree), encompassedData(data) {}
DTree::ValueNode::ValueNode(std::weak_ptr<DTree::DecisionNode> parent, std::vector<DataFrame> data) : Node(parent), encompassedData(data) {}
const std::vector<DataFrame>& DTree::ValueNode::getEncompassedData() { return encompassedData; }
std::string DTree::ValueNode::modeLabel() {
	std::map<std::string, int> res;
	for (DataFrame frame : this->encompassedData) {
		res.try_emplace(frame.label, 0);
		res[frame.label]++;
	}
	std::pair<std::string, int> max = { "No Data Encompassed", -1 };
	for (std::pair<std::string, int> e : res) {
		if (e.second > max.second) { max = e; }
	}
	return max.first;
}
std::map<std::string, int> DTree::ValueNode::proportionedLabel() {
	std::map<std::string, int> res;
	for (DataFrame frame : this->encompassedData) {
		res.try_emplace(frame.label, 0);
		res[frame.label]++;
	}
	return res;
}
std::string DTree::ValueNode::toString() {
	std::string label = modeLabel();
	return label == "No Data Encompassed" ? "None" : label;
}

DTree::DTree(std::vector<DataFrame> data_points) {
	this->nodes = {};
	auto new_node = std::make_shared<ValueNode>(*this, data_points);
	this->head = new_node;
	this->nodes.insert(new_node);
}
std::string DTree::to_string() {
	std::stringstream out;

	const std::string HBEAM = "━";
	const std::string TBEAM = "┻";
	const std::string RDBEAM = "┓";
	const std::string LDBEAM = "┏";
	const std::string RUBEAM = "┛";
	const std::string LUBEAM = "┗";

	auto root = this->head.lock();

	if (root == nullptr) {
		out << "Empty tree" << std::endl;
		return out.str();
	}

	// Calculate height of tree
	int height = 0;
	std::queue<std::pair<std::shared_ptr<Node>, int>> q;
	q.push({ root, 0 });

	while (!q.empty()) {
		auto [node, level] = q.front();
		q.pop();

		height = std::max(height, level);

		if (auto left = node->left_child.lock()) {
			q.push({ left, level + 1 });
		}

		if (auto right = node->right_child.lock()) {
			q.push({ right, level + 1 });
		}
	}

	// Map to store string representation of each node
	std::unordered_map<Node*, std::string> nodeStrings;

	// Map to store position information for each node
	std::unordered_map<Node*, int> xPositions;

	// Calculate widths at each level and node positions
	std::vector<std::vector<std::shared_ptr<Node>>> levels(height + 1);
	q.push({ root, 0 });
	int maxNodeWidth = 0;

	while (!q.empty()) {
		auto [node, level] = q.front();
		q.pop();

		// Store node at its level
		levels[level].push_back(node);

		// Calculate string representation and update max width
		std::string nodeStr = node->toString();

		nodeStrings[node.get()] = nodeStr;
		maxNodeWidth = std::max(maxNodeWidth, static_cast<int>(nodeStr.length()));

		if (auto left = node->left_child.lock()) {
			q.push({ left, level + 1 });
		}

		if (auto right = node->right_child.lock()) {
			q.push({ right, level + 1 });
		}
	}

	// Make sure we have at least 1 space between nodes
	int nodeSpacing = maxNodeWidth + 1;

	// Calculate positions for each node based on its index in a perfect binary tree
	for (int level = 0; level <= height; level++) {
		int levelWidth = pow(2, level);
		int fullWidth = pow(2, height) * nodeSpacing;
		int gap = fullWidth / levelWidth;

		for (size_t i = 0; i < levels[level].size(); i++) {
			auto node = levels[level][i];

			// Find position in perfect tree
			size_t pos = 0;
			std::shared_ptr<Node> curr = node;
			std::vector<bool> path;

			// Trace path to root
			while (auto p = curr->parent.lock()) {
				bool isRight = (p->right_child.lock().get() == curr.get());
				path.push_back(isRight);
				curr = p;
			}

			// Calculate position from path
			size_t idx = 0;
			for (auto it = path.rbegin(); it != path.rend(); ++it) {
				idx = idx * 2 + (*it ? 1 : 0);
			}

			// Store position
			xPositions[node.get()] = (idx + 1) * gap - gap / 2;
		}
	}

	// Make sure the tree is wide enough
	int totalWidth = pow(2, height) * nodeSpacing;

	// Print the tree level by level
	for (int level = 0; level <= height; level++) {
		// Print node values
		std::string nodeLine(totalWidth, ' ');

		for (auto& node : levels[level]) {
			std::string nodeStr = nodeStrings[node.get()];
			int centerPos = xPositions[node.get()];
			int startPos = centerPos - nodeStr.length() / 2;

			// Make sure we don't go out of bounds
			if (startPos >= 0 && startPos + nodeStr.length() <= nodeLine.length()) {
				for (size_t i = 0; i < nodeStr.length(); i++) {
					nodeLine[startPos + i] = nodeStr[i];
				}
			}
		}

		out << nodeLine << std::endl;

		// Skip connection lines for the last level
		if (level == height) {
			break;
		}

		// Print connection lines
		std::vector<std::string> connectorLine(totalWidth, " ");

		for (auto& node : levels[level]) {
			int nodePos = xPositions[node.get()];
			auto leftChild = node->left_child.lock();
			auto rightChild = node->right_child.lock();

			if (leftChild && rightChild) {
				int leftPos = xPositions[leftChild.get()];
				int rightPos = xPositions[rightChild.get()];

				// Safety checks
				if (nodePos >= 0 && nodePos < connectorLine.size() &&
					leftPos >= 0 && leftPos < connectorLine.size() &&
					rightPos >= 0 && rightPos < connectorLine.size()) {

					connectorLine[leftPos] = LDBEAM;
					connectorLine[rightPos] = RDBEAM;

					for (int i = leftPos + 1; i < nodePos; i++) {
						if (i >= 0 && i < connectorLine.size()) {
							connectorLine[i] = HBEAM;
						}
					}

					for (int i = nodePos + 1; i < rightPos; i++) {
						if (i >= 0 && i < connectorLine.size()) {
							connectorLine[i] = HBEAM;
						}
					}

					connectorLine[nodePos] = TBEAM;
				}
			}
			else if (leftChild) {
				int leftPos = xPositions[leftChild.get()];

				// Safety checks
				if (nodePos >= 0 && nodePos < connectorLine.size() &&
					leftPos >= 0 && leftPos < connectorLine.size()) {

					connectorLine[leftPos] = LDBEAM;

					for (int i = leftPos + 1; i < nodePos; i++) {
						if (i >= 0 && i < connectorLine.size()) {
							connectorLine[i] = HBEAM;
						}
					}

					connectorLine[nodePos] = RUBEAM;
				}
			}
			else if (rightChild) {
				int rightPos = xPositions[rightChild.get()];

				// Safety checks
				if (nodePos >= 0 && nodePos < connectorLine.size() &&
					rightPos >= 0 && rightPos < connectorLine.size()) {

					connectorLine[rightPos] = RDBEAM;

					for (int i = nodePos; i < rightPos; i++) {
						if (i >= 0 && i < connectorLine.size()) {
							connectorLine[i] = HBEAM;
						}
					}

					connectorLine[nodePos] = LUBEAM;
				}
			}
		}

		// Print vertical connectors
		for (auto& node : levels[level]) {
			if (node->left_child.lock() || node->right_child.lock()) {
				int pos = xPositions[node.get()];
				if (pos >= 0 && pos < connectorLine.size()) {
					if (connectorLine[pos] == LDBEAM || connectorLine[pos] == RDBEAM) {
						connectorLine[pos] = TBEAM;
					}
				}
			}
		}
		std::ostringstream imploded;
		std::copy(connectorLine.begin(), connectorLine.end(), std::ostream_iterator<std::string>(imploded, ""));
		out << imploded.str() << std::endl;
	}
	return out.str();
}
void DTree::split_leaf(std::shared_ptr<ValueNode> leaf, size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst) {
	std::shared_ptr<DecisionNode> new_node_ptr = std::make_shared<DecisionNode>((DTree&)*this, dataFrameField, comparison, compareAgainst);
	DecisionNode* new_node = new_node_ptr.get();
	ValueNode* old_node = leaf.get();
	if (old_node->parent.lock()->left_child.lock() == leaf) {
		//Replacing a left child
		old_node->parent.lock()->left_child = new_node_ptr;
	}
	else {
		//Replacing a right child
		old_node->parent.lock()->right_child = new_node_ptr;
	}
	nodes.erase(leaf); //Remove the 'owning' shared_pointer, so any remaining ones can decay away and the node is deallocated.
	nodes.insert(new_node_ptr);
};
	
