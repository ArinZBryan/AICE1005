#include <iostream>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

#include "DTree.h"
#include "DForest.h"
#include "utility.h"

#include "DebugLogger.hpp"

void trainDForestFromBinaryData() {
    std::vector<DataFrame> training = DTree::Training::loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\training.dat");
    DEBUG_LOG("Loaded " << training.size() << " DataFrames from file." << std::endl);
    std::vector<DataFrame> testing = DTree::Training::loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\testing.dat");
    DEBUG_LOG("Loaded " << testing.size() << " DataFrames from file." << std::endl);

    DForest forest(training);

    auto start = std::chrono::high_resolution_clock::now(); // Start timing
    forest.trainBlindForest(
        1000,
        6,
        DTree::Training::LossFunctions::entropy,
        2,
        DTree::Training::LimitingFactor::decisions,
        10,
        false,
        false
    );
    auto end = std::chrono::high_resolution_clock::now(); // End timing
    std::chrono::duration<double> elapsed = end - start; // Calculate elapsed time

    std::cout << "Trained " << forest.num_trees() << " trees in " << elapsed.count() << " seconds." << std::endl;

    start = std::chrono::high_resolution_clock::now();
    auto vec_refs = vecrefs(testing);
    std::cout << "Generated vector element references" << std::endl;
    double correct = forest.test(vec_refs);
    end = std::chrono::high_resolution_clock::now(); // End timing
    elapsed = end - start; // Calculate elapsed time

    std::cout << "Tested " << forest.num_trees() << " trees in " << elapsed.count() << " seconds." << std::endl;
    std::cout << "Accuracy: " << correct << std::endl;
}

void trainDTreeFromBinaryData() {
    std::vector<DataFrame> training = DTree::Training::loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\training.dat");
    DEBUG_LOG("Loaded " << training.size() << " DataFrames from file." << std::endl);
    std::vector<DataFrame> testing = DTree::Training::loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\testing.dat");
    DEBUG_LOG("Loaded " << testing.size() << " DataFrames from file." << std::endl);

    DTree tree(training);

    auto start = std::chrono::high_resolution_clock::now(); // Start timing
    tree.train(
        DTree::Training::LossFunctions::entropy,
        2,
        DTree::Training::LimitingFactor::leaves,
        11,
        false,
        false
    );
    auto end = std::chrono::high_resolution_clock::now(); // End timing
    std::chrono::duration<double> elapsed = end - start; // Calculate elapsed time

    std::cout << "Trained " << 1 << " trees in " << elapsed.count() << " seconds." << std::endl;

    start = std::chrono::high_resolution_clock::now();
    auto vec_refs = vecrefs(testing);
    std::cout << "Generated vector element references";
    double correct = tree.test(vec_refs);
    end = std::chrono::high_resolution_clock::now(); // End timing
    elapsed = end - start; // Calculate elapsed time

    std::cout << "Tested " << 1 << " trees in " << elapsed.count() << " seconds." << std::endl;
    std::cout << "Accuracy: " << correct << std::endl;

    std::cout << tree.to_string() << std::endl;;
}

void compareDTreeLimitingFactors() {
    std::vector<DataFrame> training = DTree::Training::loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\training.dat");
    DEBUG_LOG("Loaded " << training.size() << " DataFrames from file." << std::endl);
    std::vector<DataFrame> testing = DTree::Training::loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\testing.dat");
    DEBUG_LOG("Loaded " << testing.size() << " DataFrames from file." << std::endl);
    std::vector<const DataFrame*> testing_refs = vecrefs(testing);
    DEBUG_LOG("Generated testing references.");

    std::cout << "Decisions\tAccuracy" << std::endl;
 
    for (int i = 0; i < 50; i++) {
        DTree tree(training);
        tree.train(
            DTree::Training::LossFunctions::entropy,
            2,
            DTree::Training::LimitingFactor::decisions,
            i,
            false,
            false
        );
        double accuracy = tree.test(testing_refs);
        std::cout << i << "\t" << accuracy << std::endl;
    }
}

void setupConsoleForUTF8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
        ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    std::locale::global(std::locale("en_US.UTF-8"));
    std::cout.imbue(std::locale());
#endif
}

int main()
{
    setupConsoleForUTF8();

    compareDTreeLimitingFactors();

}

