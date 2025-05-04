#pragma once
#include <vector>
#include <string>

// Like strtok from c stdlib, but for std::string
static std::vector<std::string> strtokstr(std::string s, char sep);

double getRandomDouble();