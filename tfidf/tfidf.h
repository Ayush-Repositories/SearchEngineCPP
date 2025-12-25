#ifndef TFIDF_H
#define TFIDF_H
#include <string>
#include <vector>
#include "../preprocess/preprocess.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <queue>

extern size_t totalDocuments;
extern std::unordered_map<std::string, std::unordered_map<std::string, int>> invertedIndex;

void buildIndex(const std::string& directory);
std::vector<std::pair<std::string, double>>rankTFIDF(const std::string& query, size_t topN);

#endif