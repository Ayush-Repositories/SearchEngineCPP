#ifndef TFIDF_H
#define TFIDF_H
#include <string>
#include <vector>

extern size_t totalDocuments;

std::unordered_map<std::string, std::unordered_map<std::string, int>> invertedIndex;
void buildIndex(const std::string& directory);
std::vector<std::pair<std::string, double>>rankTFIDF(const std::string& query, size_t topN);

#endif