#pragma once
#include <vector>
#include <string>

// Like strtok from c stdlib, but for std::string
std::vector<std::string> strtokstr(std::string s, char sep);

double getRandomDouble();

// This is in the header, because it is a template, and so will fail to link
// properly if the code is in utility.cpp and the declaration in utility.h
template<typename T>
std::vector<const T*> vecrefs(const std::vector<T>& orig) {
    std::vector<const T*> ret;
    ret.reserve(orig.size());
    for (const T& val : orig) {
        ret.push_back(&val);
    }
    return ret;
}