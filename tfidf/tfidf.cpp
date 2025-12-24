#include "tfidf.h"
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

namespace fs = std::filesystem;

std::unordered_map<std::string, std::unordered_map<std::string, int>> invertedIndex;

std::unordered_map<std::string, std::unordered_map<std::string, double>> docTFIDF;

size_t totalDocuments = 0;

void buildIndex(const std::string& directory) {
    invertedIndex.clear();
    docTFIDF.clear();
    totalDocuments = 0;

    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;

        std::ifstream in(entry.path());
        if (!in) continue;

        totalDocuments++;
        std::string filepath = entry.path().string();
        std::string word;
        while (in >> word) {
            invertedIndex[word][filepath]++;
        }
    }

    for (const auto& [term, posting] : invertedIndex) {
        double idf = std::log((double)totalDocuments / posting.size());
        for (const auto& [doc, tf] : posting) {
            docTFIDF[doc][term] = tf * idf;
        }
    }
}

static std::unordered_map<std::string, int> queryTF(const std::string& processedQuery) {
    std::unordered_map<std::string, int> tf;
    std::stringstream ss(processedQuery);
    std::string word;
    while (ss >> word) tf[word]++;
    return tf;
}

static double cosine(
    const std::unordered_map<std::string, double>& a,
    const std::unordered_map<std::string, double>& b
) {
    double dot = 0.0, na = 0.0, nb = 0.0;
    for (const auto& [_, v] : a) na += v*v;
    for (const auto& [_, v] : b) nb += v*v;
    for (const auto& [k, v] : a) {
        auto it = b.find(k);
        if (it != b.end()) dot += v * it->second;
    }
    if (na == 0.0 || nb == 0.0) return 0.0;
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

std::vector<std::pair<std::string, double>> rankTFIDF(const std::string& query, size_t topN) {
    std::string cleanQuery = cleaner(query);
    auto qTF = queryTF(cleanQuery);

    std::unordered_map<std::string, double> qVec;
    for (const auto& [term, tf] : qTF) {
        auto it = invertedIndex.find(term);
        if (it == invertedIndex.end()) continue;
        double idf = std::log((double)totalDocuments / it->second.size());
        qVec[term] = tf * idf;
    }

    std::unordered_set<std::string> candidateDocs;
    for (const auto& [term, _] : qTF) {
        auto it = invertedIndex.find(term);
        if (it == invertedIndex.end()) continue;
        for (const auto& [doc, __] : it->second)
            candidateDocs.insert(doc);
    }

    using Pair = std::pair<double, std::string>;
    auto cmp = [](const Pair& a, const Pair& b){ return a.first > b.first; };
    std::priority_queue<Pair, std::vector<Pair>, decltype(cmp)> minHeap(cmp);

    for (const auto& doc : candidateDocs) {
        auto it = docTFIDF.find(doc);
        if (it == docTFIDF.end()) continue;
        double score = cosine(qVec, it->second);
        if (score <= 0.0) continue;

        if (minHeap.size() < topN) minHeap.emplace(score, doc);
        else if (score > minHeap.top().first) {
            minHeap.pop();
            minHeap.emplace(score, doc);
        }
    }

    std::vector<std::pair<std::string, double>> results;
    while (!minHeap.empty()) {
        results.emplace_back(minHeap.top().second, minHeap.top().first);
        minHeap.pop();
    }
    std::reverse(results.begin(), results.end());
    return results;
}