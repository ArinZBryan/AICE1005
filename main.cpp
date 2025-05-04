#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "DTree.h"

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
        ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    std::locale::global(std::locale("en_US.UTF-8"));
    std::cout.imbue(std::locale());
#endif

    //std::vector<DataFrame> dfs = loadFile(10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\training.dat");
    std::vector<DataFrame> dfs = DTree::Training::loadFileFloatRange(9, 10, "C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\gameandgrade.dat");
    DEBUG_LOG("Loaded " << dfs.size() << " DataFrames from file." << std::endl);
    
    DTree tree(dfs);
    tree.train(DTree::Training::LossFunctions::entropy, 10, DTree::Training::LimitingFactor::leaves, 6, true, false);
    
    std::cout << tree.to_string();
}

