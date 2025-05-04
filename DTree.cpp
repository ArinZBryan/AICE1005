#include "DTree.h"
#include "utility.h"

bool DataFrame::lt(const DataFrame& other, size_t field) const {
	if (std::holds_alternative<int>(this->fields[field])) {
		return std::get<int>(this->fields[field]) < std::get<int>(other.fields[field]);
	}
	else if (std::holds_alternative<float>(this->fields[field])) {
		return std::get<float>(this->fields[field]) < std::get<float>(other.fields[field]);
	}
	return false;
}

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

DTree::Node::Node(DTree& tree) : tree(tree), left_child(), right_child() {}
DTree::Node::Node(std::weak_ptr<DTree::Node> parent) : parent(parent), tree(parent.lock()->tree), left_child(), right_child() {}
DTree::Node::~Node() = default;
std::string DTree::Node::toString() { return "Node"; };
size_t DTree::Node::depth() {
	size_t d = 0;
	std::weak_ptr<DTree::Node> cur = this->parent;
	while (!cur.expired()) {
		d++;
		cur = cur.lock()->parent;
	}
	return d;
}

DTree::DecisionNode::DecisionNode(DTree& tree) : Node(tree), comparison(lessThan) {};
DTree::DecisionNode::DecisionNode(std::weak_ptr<DTree::Node> parent) : Node(parent), comparison(lessThan) {};
DTree::DecisionNode::DecisionNode(DTree& tree, size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst)
: Node(tree), dataFrameField(dataFrameField), comparison(comparison), compareAgainst(compareAgainst) {}
bool DTree::DecisionNode::decide(const DataFrame* value) const { 
	return DTree::DecisionNode::decide(this->dataFrameField, this->comparison, this->compareAgainst, value);
}
bool DTree::DecisionNode::decide(size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst, const DataFrame* value) {
	bool ret = false;
	if (comparison == DTree::DecisionNode::comparisonType::equal) {
		return compareAgainst == value->fields[dataFrameField];
	}
	else if (comparison == DTree::DecisionNode::comparisonType::lessThan) {
		return compareAgainst < value->fields[dataFrameField];
	}
	else if (comparison == DTree::DecisionNode::comparisonType::greaterThan) {
		return compareAgainst > value->fields[dataFrameField];
	}
	return false;

}
std::pair<std::vector<const DataFrame*>, std::vector<const DataFrame*>> DTree::DecisionNode::evaluate(const std::vector<const DataFrame*>& data) const {
	return DTree::DecisionNode::evaluate(this->dataFrameField, this->comparison, this->compareAgainst, data);
}
std::pair<std::vector<const DataFrame*>, std::vector<const DataFrame*>> DTree::DecisionNode::evaluate(size_t dataFrameField, DTree::DecisionNode::comparisonType comparison, std::variant<int, float> compareAgainst, const std::vector<const DataFrame*>& data) {
	std::vector<const DataFrame*> out_a;
	std::vector<const DataFrame*> out_b;
	for (const DataFrame* df : data) {
		if (decide(dataFrameField, comparison, compareAgainst, df)) {
			out_a.push_back(df);
		}
		else {
			out_b.push_back(df);
		}
	}
	return { out_a, out_b };
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
DTree::ValueNode::ValueNode(DTree& tree, std::vector<const DataFrame*> data) : Node(tree), encompassedData(data) {}
DTree::ValueNode::ValueNode(std::weak_ptr<DTree::DecisionNode> parent, std::vector<const DataFrame*> data) : Node(parent), encompassedData(data) {}
std::vector<const DataFrame*> DTree::ValueNode::getEncompassedData() const { return encompassedData; }
std::string DTree::ValueNode::modeLabel() {
	std::map<std::string, int> res = proportionedLabel();
	std::pair<std::string, int> max = { "None", -1 };
	for (std::pair<std::string, int> e : res) {
		if (e.second > max.second) { max = e; }
	}
	std::stringstream ret;
	ret << max.first;
	return ret.str();
}
std::map<std::string, int> DTree::ValueNode::proportionedLabel() {
	std::map<std::string, int> res;
	for (const DataFrame* frame : this->encompassedData) {
		res.try_emplace(frame->label, 0);
		res[frame->label]++;
	}
	return res;
}
std::string DTree::ValueNode::toString() {
	std::stringstream label;
	switch (this->tree.print_style) {
	case DTree::printStyle::none: 
		label << modeLabel(); 
		break;
	case DTree::printStyle::size: 
		label << modeLabel() << " ("  << this->encompassedData.size() << ")"; 
		break;
	case DTree::printStyle::address: 
		label << modeLabel() << " (" << this << ")"; 
		break;
	case DTree::printStyle::percent: 
		auto labels = proportionedLabel();
		std::pair<std::string, int> max = { "None", -1 };
		for (std::pair<std::string, int> e : labels) {
			if (e.second > max.second) { max = e; }
		}
		label << max.first << " (" << static_cast<float>(max.second) / static_cast<float>(this->encompassedData.size()) << ")";
		break;
	}
	return label.str();
}

std::vector<size_t> DTree::make_fields_arg(const std::vector<DataFrame>& data_points) {
	auto fields = std::vector<size_t>();
	fields.reserve(data_points[0].fields.size());
	for (size_t i = 0; i < data_points[0].fields.size(); i++) {
		fields.push_back(i);
	}
	return fields;
}

DTree::DTree(std::vector<DataFrame> data_points) : fields(make_fields_arg(data_points)), training() {

	// take ownership of the vector forcibly
	this->data = std::move(data_points);
	
	// give the new node a vector of (non-owning) pointers to the data
	std::vector<const DataFrame*> refs;
	refs.reserve(this->data.size());
	for (const DataFrame& df : this->data) { 
		refs.push_back(&df); 
	}

	// make the node and move the vector of pointers into it. We don't own it anymore.
	auto new_node = std::make_shared<ValueNode>(*this, std::move(refs));

	// finish initialising the tree
	this->head = new_node;
	this->nodes = { new_node };
}
DTree::DTree(std::vector<DataFrame> data_points, std::initializer_list<size_t> fields) : fields(fields), training() {

	// take ownership of the vector forcibly
	this->data = std::move(data_points);

	// give the new node a vector of (non-owning) pointers to the data
	std::vector<const DataFrame*> refs;
	refs.reserve(this->data.size());
	for (const DataFrame& df : this->data) {
		refs.push_back(&df);
	}

	// make the node and move the vector of pointers into it. We don't own it anymore.
	auto new_node = std::make_shared<ValueNode>(*this, std::move(refs));

	// finish initialising the tree
	this->head = new_node;
	this->nodes = { new_node };
}

std::string DTree::to_string() const {
	std::stringstream out;

	const std::string HBEAM = std::string(reinterpret_cast<const char*>(u8"━"));
	const std::string TBEAM = std::string(reinterpret_cast<const char*>(u8"┻"));
	const std::string RDBEAM = std::string(reinterpret_cast<const char*>(u8"┓"));
	const std::string LDBEAM = std::string(reinterpret_cast<const char*>(u8"┏"));
	const std::string RUBEAM = std::string(reinterpret_cast<const char*>(u8"┛"));
	const std::string LUBEAM = std::string(reinterpret_cast<const char*>(u8"┗"));

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
	std::shared_ptr<DecisionNode> new_decision_node = std::make_shared<DecisionNode>((DTree&)*this, dataFrameField, comparison, compareAgainst);
	auto new_data = new_decision_node->evaluate(leaf->getEncompassedData());

	if (new_data.first.size() == 0 || new_data.second.size() == 0) {
		throw "Split will result in empty leaf";
	}

	std::shared_ptr<ValueNode> new_value_node_a = std::make_shared<ValueNode>(new_decision_node, new_data.first);
	std::shared_ptr<ValueNode> new_value_node_b = std::make_shared<ValueNode>(new_decision_node, new_data.second);
	new_decision_node->left_child = new_value_node_a;
	new_decision_node->right_child = new_value_node_b;

	if (leaf->parent.expired()) { // Splitting the head Node
		this->head = new_decision_node;
	} else if (leaf->parent.lock()->left_child.lock() == leaf) { //Replacing a left child
		leaf->parent.lock()->left_child = new_decision_node;
		new_decision_node->parent = leaf->parent;
	} else { //Replacing a right child
		leaf->parent.lock()->right_child = new_decision_node;
		new_decision_node->parent = leaf->parent;
	}

	nodes.erase(leaf); //Remove the 'owning' shared_pointer, so any remaining ones can decay away and the node is deallocated.
	nodes.insert(new_decision_node);
	nodes.insert(new_value_node_a);
	nodes.insert(new_value_node_b);
};
size_t DTree::max_depth() {
	std::queue<std::pair<std::weak_ptr<DTree::Node>, size_t>> toCheck({ {this->head, 0} });
	size_t max_depth = 0;
	while (toCheck.size() > 0) {
		std::pair<std::weak_ptr<DTree::Node>, size_t> cur = toCheck.front();
		toCheck.pop();
		if (!cur.first.lock()->left_child.expired()) { toCheck.push({ cur.first.lock()->left_child, cur.second + 1 }); }
		if (!cur.first.lock()->right_child.expired()) { toCheck.push({ cur.first.lock()->right_child, cur.second + 1 }); }
		if (cur.second > max_depth) { max_depth = cur.second; }
	}
	return max_depth;
}
std::vector<std::weak_ptr<DTree::ValueNode>> DTree::get_leaves() {
	std::vector<std::weak_ptr<DTree::ValueNode>> ret;
	std::queue<std::weak_ptr<DTree::Node>> toCheck({ this->head });
	while (toCheck.size() > 0) {
		std::weak_ptr<DTree::Node> cur = toCheck.front();
		toCheck.pop();
		if (cur.lock()->left_child.expired() && cur.lock()->right_child.expired()) {
			ret.push_back(std::dynamic_pointer_cast<DTree::ValueNode>(cur.lock()));
		}
		else {
			if (!cur.lock()->left_child.expired()) { toCheck.push(cur.lock()->left_child); }
			if (!cur.lock()->right_child.expired()) { toCheck.push(cur.lock()->right_child); }
		}
	}
	return ret;
}

std::vector<DataFrame> DTree::Training::loadFile(size_t label, std::filesystem::path path) {
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
		bool labelIsNumeric = false;
		for (size_t i = 0; i < tokens.size(); i++) {
			std::string token = tokens[i];
			if (i == label) {
				df.label = token;
			}
			else if (token.find(".") != std::string::npos) {
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
	if (ret[0].label == "") {
		std::cerr << "Could not autodetect labels." << std::endl;
		exit(1);
	}
	return ret;
}
std::vector<DataFrame> DTree::Training::loadFileIntRange(size_t label, unsigned int no_ranges, std::filesystem::path path) {
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
        bool labelIsNumeric = false;
        for (size_t i = 0; i < tokens.size(); i++) {
            std::string token = tokens[i];
            if (i == label) {
                df.label = token;
            }
            else if (token.find(".") != std::string::npos) {
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
    int min = INT_MIN;
    int max = INT_MAX;
    for (const DataFrame& df : ret) {
        int val = strtol(df.label.c_str(), nullptr, 10);
        if (val > max) { max = val; }
        if (val < min) { min = val; }
    }

    std::vector<std::tuple<std::string, int, int>> labels(no_ranges);
    labels[0] = {
        "x < " + std::to_string(static_cast<int>(min + ((max - min) / static_cast<float>(no_ranges)))),
        INT_MIN,
        static_cast<int>(min + ((max - min) / static_cast<float>(no_ranges)))
    };
    labels[no_ranges - 1] = {
        std::to_string(static_cast<int>(min + ((max - min) * static_cast<float>(no_ranges - 1) / static_cast<float>(no_ranges)))) + " <= x",
        static_cast<int>(min + ((max - min) * static_cast<float>(no_ranges - 1) / static_cast<float>(no_ranges))),
        INT_MAX
    };
    for (unsigned int i = 1; i < no_ranges - 1; i++) {
        float bound_lo = min + ((max - min) * static_cast<float>(i) / static_cast<float>(no_ranges));
        float bound_hi = min + ((max - min) * static_cast<float>(i + 1) / static_cast<float>(no_ranges));
        labels[i] = { std::to_string(bound_lo) + " <= x < " + std::to_string(bound_hi), bound_lo, bound_hi };
    }

    for (DataFrame& df : ret) {
        int val = strtol(df.label.c_str(), nullptr, 10);
        for (unsigned int i = 0; i < labels.size(); i++) {
            if (std::get<1>(labels[i]) <= val && val < std::get<2>(labels[i])) {
                df.label = std::get<0>(labels[i]);
            }
        }
    }

    return ret;
}
std::vector<DataFrame> DTree::Training::loadFileFloatRange(size_t label, unsigned int no_ranges, std::filesystem::path path) {
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
        bool labelIsNumeric = false;
        for (size_t i = 0; i < tokens.size(); i++) {
            std::string token = tokens[i];
            if (i == label) {
                df.label = token;
            }
            else if (token.find(".") != std::string::npos) {
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

    float min = std::numeric_limits<float>::infinity();
    float max = -std::numeric_limits<float>::infinity();
    for (const DataFrame& df : ret) {
        float val = strtof(df.label.c_str(), nullptr);
        if (val > max) { max = val; }
        if (val < min) { min = val; }
    }

    std::vector<std::tuple<std::string, float, float>> labels(no_ranges);
    labels[0] = {
        "x < " + std::to_string(min + ((max - min) / static_cast<float>(no_ranges))),
        -std::numeric_limits<float>::infinity(),
        min + ((max - min) / static_cast<float>(no_ranges))
    };
    labels[no_ranges - 1] = {
        std::to_string(min + ((max - min) * static_cast<float>(no_ranges - 1) / static_cast<float>(no_ranges))) + " <= x",
        min + ((max - min) * static_cast<float>(no_ranges - 1) / static_cast<float>(no_ranges)),
        std::numeric_limits<float>::infinity()
    };
    for (unsigned int i = 1; i < no_ranges - 1; i++) {
        float bound_lo = min + ((max - min) * static_cast<float>(i) / static_cast<float>(no_ranges));
        float bound_hi = min + ((max - min) * static_cast<float>(i + 1) / static_cast<float>(no_ranges));
        labels[i] = { std::to_string(bound_lo) + " <= x < " + std::to_string(bound_hi), bound_lo, bound_hi };
    }



    for (DataFrame& df : ret) {
        float val = strtof(df.label.c_str(), nullptr);
        for (unsigned int i = 0; i < labels.size(); i++) {
            if (std::get<1>(labels[i]) <= val && val < std::get<2>(labels[i])) {
                df.label = std::get<0>(labels[i]);
            }
        }
    }

    return ret;
}
double DTree::Training::LossFunctions::entropy(std::vector<const DataFrame*> data) {
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
        if (p == 0.0) { continue; }
        // Note: This (above line) is to fix a bug where silent NaNs can creep in if one class is not represented
        //       because 0 * log2(0) = 0 * NaN (silent), we get another silent NaN instead of zero,
        //       because where humans would just not continue calculating, computers continue anyway.
        entropy += p * log2(p);
    }
    return -entropy;
}
double DTree::Training::LossFunctions::giniImpurity(std::vector<const DataFrame*> data) {
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
double DTree::Training::evaluateSplit(
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
                }
                else {
                    data_b.push_back(data[i]);
                }
            }
            else if (std::holds_alternative<float>(compareAgainst)) {
                if (std::get<float>(compareAgainst) == std::get<float>(data[i]->fields[comparisonField])) {
                    data_a.push_back(data[i]);
                }
                else {
                    data_b.push_back(data[i]);
                }
            }
            break;
        case DTree::DecisionNode::lessThan:
            if (std::holds_alternative<int>(compareAgainst)) {
                if (std::get<int>(data[i]->fields[comparisonField]) < std::get<int>(compareAgainst)) {
                    data_a.push_back(data[i]);
                }
                else {
                    data_b.push_back(data[i]);
                }
            }
            else if (std::holds_alternative<float>(compareAgainst)) {
                if (std::get<float>(data[i]->fields[comparisonField]) < std::get<float>(compareAgainst)) {
                    data_a.push_back(data[i]);
                }
                else {
                    data_b.push_back(data[i]);
                }
            }
            break;
        case DTree::DecisionNode::greaterThan:
            if (std::holds_alternative<int>(compareAgainst)) {
                if (std::get<int>(data[i]->fields[comparisonField]) > std::get<int>(compareAgainst)) {
                    data_a.push_back(data[i]);
                }
                else {
                    data_b.push_back(data[i]);
                }
            }
            else if (std::holds_alternative<float>(compareAgainst)) {
                if (std::get<float>(data[i]->fields[comparisonField]) > std::get<float>(compareAgainst)) {
                    data_a.push_back(data[i]);
                }
                else {
                    data_b.push_back(data[i]);
                }
            }
            break;
        default:
            break;
        }
    }

    if (data_a.size() < 1 || data_b.size()) {
        return 0.0;
    }

    double loss_a = minimiseFunc(data_a);
    double loss_b = minimiseFunc(data_b);
    double fraction_a = static_cast<double>(data_a.size()) / static_cast<double>(data.size());
    double fraction_b = static_cast<double>(data_b.size()) / static_cast<double>(data.size());

    return minimiseFunc(data) - (fraction_a * loss_a + fraction_b * loss_b);
}

/*
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
*/

// This function, based on some preliminary profiling runs 2.3x faster due to utilising OpenMP
// to multithread the main for loop. And it was so much easier to make work than python 
// multithreading! Yipee!
std::priority_queue<struct DTree::Training::SplitDetails> DTree::Training::findBestSplits(
    std::shared_ptr<DTree::ValueNode> src,
    double(*evaluationFunction)(std::vector<const DataFrame*>),
    unsigned int samples,
    bool continuousInts,
    bool useGreaterThan
) {
    std::vector<const DataFrame*> data = src->getEncompassedData();
    if (data.size() == 0) { throw "Cannot split node with no data"; }
    size_t df_fields = data[0]->fields.size();

    // Create thread-local priority queues to avoid race conditions
    std::vector<std::priority_queue<struct SplitDetails>> thread_queues(
#ifdef USE_OPENMP
        omp_get_max_threads()
#else
        1
#endif
    );

#ifdef USE_OPENMP
    // Parallelize the loop over field indices
#pragma omp parallel for
#endif
    for (int field_index = 0; field_index < df_fields; field_index++) {
        // Get the thread-local queue for this thread
        int thread_id =
#ifdef USE_OPENMP
            omp_get_thread_num();
#else
            0
#endif
            std::priority_queue<struct SplitDetails>&local_queue = thread_queues[thread_id];

        std::vector<const DataFrame*> temp_copy = data;
        const DataFrame* min_element = nullptr;
        const DataFrame* max_element = nullptr;
        for (const DataFrame* elem : temp_copy) {
            if (min_element == nullptr || elem->fields[field_index] < min_element->fields[field_index]) { min_element = elem; }
            if (max_element == nullptr || elem->fields[field_index] > min_element->fields[field_index]) { max_element = elem; }
        }

        if (min_element == max_element) {
            local_queue.push({
                -std::numeric_limits<double>::infinity(),
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
                auto purityGain = evaluateSplit(
                    src,
                    DTree::DecisionNode::comparisonType::equal,
                    static_cast<size_t>(field_index),
                    j,
                    evaluationFunction
                );
                local_queue.push({
                    purityGain,
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
#ifdef USE_OPENMP
    for (auto& local_queue : thread_queues) {
        while (!local_queue.empty()) {
            ret.push(local_queue.top());
            local_queue.pop();
        }
    }
#else
    ret = thread_queues[0];
#endif
    return ret;
}

void DTree::Training::train(
    DTree& tree,
    double(*evaluationFunction)(std::vector<const DataFrame*>),
    unsigned int samples,
    DTree::Training::LimitingFactor limiting_factor,
    unsigned int limit,
    bool continuousInts,
    bool useGreaterThan
) {
    if (limiting_factor == DTree::Training::LimitingFactor::decisions) {
        std::priority_queue<DTree::Training::NodeSplitDetails> splits;
        for (int i = 0; i < limit; i++) {
            for (auto leaf : tree.get_leaves()) {
                if (leaf.lock()->getEncompassedData().size() < 2) {
                    DEBUG_LOG("Prevented splitting small leaf." << std::endl);
                }
                else {
                    auto best_splits = DTree::Training::findBestSplits(leaf.lock(), evaluationFunction, samples, continuousInts, useGreaterThan);
                    DEBUG_LOG("Evaluated " << best_splits.size() << " splits on node @ " << leaf.lock().get() << std::endl);
                    splits.push({ best_splits.top(), leaf });
                }
            }

            DEBUG_LOG("Found " << splits.size() << " splits..." << std::endl);
            if (splits.size() == 0) { break; }

            auto split = splits.top(); splits.pop();
            while (split.second.expired() && splits.size() > 0) {
                split = splits.top(); splits.pop();
                DEBUG_LOG("Rejected split - leaf expired or too deep. Trying another..." << std::endl);
            }

            if (!split.second.expired()) {
                DEBUG_LOG("Spliting on field " << split.first.comparison.df_field << " (" << split.second.lock()->toString() << ") ");
                DEBUG_LOG("Purity Gain: " << split.first.purity << std::endl);
                tree.split_leaf(split.second.lock(), split.first.comparison.df_field, split.first.comparison.type, split.first.comparison.constant);
                DEBUG_LOG(tree.to_string());
            }
            else {
                DEBUG_LOG("Could not split - all splits were rejected." << std::endl);
                break;
            }
        }
        return;
    }
    else if (limiting_factor == DTree::Training::LimitingFactor::depth) {
        std::priority_queue<DTree::Training::NodeSplitDetails> splits;
        while (tree.max_depth() <= limit) {
            for (auto leaf : tree.get_leaves()) {
                if (leaf.lock()->getEncompassedData().size() < 2) {
                    DEBUG_LOG("Prevented splitting small leaf." << std::endl);
                }
                else {
                    auto best_splits = DTree::Training::findBestSplits(leaf.lock(), evaluationFunction, samples, continuousInts, useGreaterThan);
                    DEBUG_LOG("Evaluated " << best_splits.size() << " splits on node @ " << leaf.lock().get() << std::endl);
                    splits.push({ best_splits.top(), leaf });
                }
            }

            DEBUG_LOG("Found " << splits.size() << " splits..." << std::endl);
            if (splits.size() == 0) { break; }

            auto split = splits.top(); splits.pop();
            while ((split.second.expired() || split.second.lock()->depth() >= limit) && splits.size() > 0) {
                split = splits.top(); splits.pop();
                DEBUG_LOG("Rejected split - leaf expired or too deep. Trying another..." << std::endl);
            }

            if (!(split.second.expired() || split.second.lock()->depth() >= limit)) {
                DEBUG_LOG("Spliting on field " << split.first.comparison.df_field << " (" << split.second.lock()->toString() << ") ");
                DEBUG_LOG("Purity Gain: " << split.first.purity << std::endl);
                tree.split_leaf(split.second.lock(), split.first.comparison.df_field, split.first.comparison.type, split.first.comparison.constant);
                DEBUG_LOG(tree.to_string());
            }
            else {
                DEBUG_LOG("Could not split - all splits were rejected." << std::endl);
                break;
            }
        }
        return;
    }
    else if (limiting_factor == DTree::Training::LimitingFactor::leaves) {
        std::priority_queue<DTree::Training::NodeSplitDetails> splits;
        while (tree.get_leaves().size() < limit) {
            for (auto leaf : tree.get_leaves()) {
                if (leaf.lock()->getEncompassedData().size() < 2) {
                    DEBUG_LOG("Prevented splitting small leaf." << std::endl);
                }
                else {
                    auto best_splits = DTree::Training::findBestSplits(leaf.lock(), evaluationFunction, samples, continuousInts, useGreaterThan);
                    DEBUG_LOG("Evaluated " << best_splits.size() << " splits on node @ " << leaf.lock().get() << std::endl);
                    splits.push({ best_splits.top(), leaf });
                }
            }

            DEBUG_LOG("Found " << splits.size() << " splits..." << std::endl);
            if (splits.size() == 0) { break; }

            auto split = splits.top(); splits.pop();
            while (split.second.expired() && splits.size() > 0) {
                split = splits.top(); splits.pop();
                DEBUG_LOG("Rejected split - leaf expired or too deep. Trying another..." << std::endl);
            }

            if (!split.second.expired()) {
                DEBUG_LOG("Spliting on field " << split.first.comparison.df_field << " (" << split.second.lock()->toString() << ") ");
                DEBUG_LOG("Purity Gain: " << split.first.purity << std::endl);
                tree.split_leaf(split.second.lock(), split.first.comparison.df_field, split.first.comparison.type, split.first.comparison.constant);
                DEBUG_LOG(tree.to_string());
            }
            else {
                DEBUG_LOG("Could not split - all splits were rejected." << std::endl);
                break;
            }
        }
        return;
    }
    else {
        return;
    }
}

void DTree::train(
    double(*evaluationFunction)(std::vector<const DataFrame*>),
    unsigned int samples,
    DTree::Training::LimitingFactor limiting_factor,
    unsigned int limit,
    bool continuousInts,
    bool useGreaterThan
) {
    return DTree::Training::train(
        *this,
        evaluationFunction,
        samples,
        limiting_factor,
        limit,
        continuousInts,
        useGreaterThan
    );
}