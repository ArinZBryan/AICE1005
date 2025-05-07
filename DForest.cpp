#include <random>
#include <algorithm>
#include <vector>

#include "DForest.h"

#ifdef USE_OPENMP
#include <omp.h>
#endif

DForest::DForest(std::vector<DataFrame> data) : data(data), trees() {}

void DForest::trainBlindForest(
    size_t size,
    unsigned int num_hidden_fields,
    double(*evaluationFunction)(std::vector<const DataFrame*>),
    unsigned int samples,
    DTree::Training::LimitingFactor limiting_factor,
    unsigned int limit,
    bool continuousInts,
    bool useGreaterThan
) {
    // allocate space for non-owning trees
    this->trees.reserve(size);

    // make vector of pointers for non-owning trees
    std::vector<const DataFrame*> data_refs;
    data_refs.reserve(this->data.size());
    for (const DataFrame& df : this->data) {
        data_refs.push_back(&df);
    }

    // make vector of all valid field indeces. Copied from DTree::make_fields_arg
    auto all_fields = std::vector<size_t>();
    all_fields.reserve(data[0].fields.size());
    for (size_t i = 0; i < data[0].fields.size(); i++) {
        all_fields.push_back(i);
    }

    // randomly remove some of the indeces to 'hide' them from the created tree.
    std::random_device rd;
    std::mt19937 gen(rd());
    for (size_t i = 0; i < size; i++) {
        auto this_fields = std::vector<size_t>(all_fields);
        std::shuffle(this_fields.begin(), this_fields.end(), gen);
        this_fields.erase(this_fields.end() - num_hidden_fields, this_fields.end());

        // we don't need the list of fields the tree will use, so we may as well force a move here if we can.
        this->trees.emplace_back(data_refs, std::move(this_fields));
    }

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < this->trees.size(); i++) {
        this->trees[i].train(
            evaluationFunction,
            samples,
            limiting_factor,
            limit,
            continuousInts,
            useGreaterThan
        );
    }

}

std::string DForest::classify(const DataFrame* df) {
    std::map<std::string, int> votes;
    for (int i = 0; i < this->trees.size(); i++) {
        std::string vote = this->trees[i].classify(df);
        votes.try_emplace(vote, 0);
        votes[vote]++;
    }
    std::pair<std::string, int> max = { "", -1 };
    for (std::pair<std::string, int> vote_class : votes) {
        if (vote_class.second == -1) {
            max = vote_class;
        }
        else if (vote_class.second > max.second) {
            max = vote_class;
        }
    }
    return max.first;
}
std::vector<std::string> DForest::classify(const std::vector<const DataFrame*>& data) {
    std::vector<std::string> ret;
    ret.reserve(data.size());
    for (const DataFrame* df : data) {
        ret.push_back(this->classify(df));
    }
    return ret;
}
bool DForest::test(const DataFrame* df) {
    return df->label == this->classify(df);
}
double DForest::test(std::vector<const DataFrame*> data) {
    double correct = 0.0;
    double incorrect = 0.0;
#pragma omp parallel for reduction(+:correct, incorrect)
    for (int i = 0; i < static_cast<int>(data.size()); i++) {
        if (this->test(data[i])) {
            correct += 1.0;
        }
        else {
            incorrect += 1.0;
        }
    }
    return correct / (correct + incorrect);
}

size_t DForest::data_size() {
    return this->data.size();
}

size_t DForest::num_trees() {
    return this->trees.size();
}