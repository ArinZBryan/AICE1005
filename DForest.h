#pragma once

#include "DTree.h"

class DForest
{
public:
    /*
	void trainTemperateForest(
        size_t forest_size,
        double forest_temperature,
        std::vector<DataFrame> data,
        double(*evaluationFunction)(std::vector<const DataFrame*>),
        unsigned int samples,
        DTree::training::LimitingFactor limiting_factor,
        unsigned int limit,
        bool continuousInts,
        bool useGreaterThan
    );
    */

    DForest(std::vector<DataFrame> data);

	void trainBlindForest(
        size_t size, 
        unsigned int num_hidden_fields,
        double(*evaluationFunction)(std::vector<const DataFrame*>),
        unsigned int samples,
        DTree::Training::LimitingFactor limiting_factor,
        unsigned int limit,
        bool continuousInts = false,
        bool useGreaterThan = false
    );

    std::string classify(const DataFrame* df);
    std::vector<std::string> classify(const std::vector<const DataFrame*>& data);
    bool test(const DataFrame* df);
    double test(std::vector<const DataFrame*> data);
    
    size_t data_size();
    size_t num_trees();
private:
	std::vector<DataFrame> data;
	std::vector<DTree_Weak> trees;
};

