#include "utility.h"
#include <sstream>
#include <random>

// Like strtok from c stdlib, but for std::string
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

double getRandomDouble() {
    static std::random_device rd; 
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution dis(0.0, 1.0); 
    return dis(gen);
}