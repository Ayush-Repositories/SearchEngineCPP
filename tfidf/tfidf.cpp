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
#include <thread>
#include <mutex>
#include <atomic>

namespace fs = std::filesystem;

std::unordered_map<std::string, std::unordered_map<std::string, int>> invertedIndex;
std::unordered_map<std::string, std::unordered_map<std::string, double>> docTFIDF;

size_t totalDocuments = 0;

void buildIndex(const std::string& directory) {
    invertedIndex.clear();
    docTFIDF.clear();
    totalDocuments = 0;

    std::vector<std::filesystem::path> filePaths;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            filePaths.push_back(entry.path());
        }
    }

    if (filePaths.empty()) {
        return;
    }

    size_t numThreads = std::max(1u, std::thread::hardware_concurrency());
    numThreads = std::min(numThreads, filePaths.size());

    std::atomic<size_t> docCount{0};

    std::vector<std::unordered_map<std::string, std::unordered_map<std::string, int>>> partialIndexes(numThreads);
    std::mutex errorMutex;

    auto processFiles = [&](size_t threadId, size_t start, size_t end) {
        auto& partialIndex = partialIndexes[threadId];
        size_t localDocCount = 0;

        for (size_t i = start; i < end; ++i) {
            const auto& filepath = filePaths[i];
            std::string filepathStr = filepath.string();
            
            std::ifstream in(filepath, std::ios::binary);
            if (!in) {
                std::lock_guard<std::mutex> lock(errorMutex);
                std::cerr << "Failed to open file for indexing: " << filepathStr << std::endl;
                continue;
            }

            std::ostringstream buffer;
            buffer << in.rdbuf();
            std::string content = buffer.str();
            in.close();

            if (content.empty()) continue;

            localDocCount++;
            
            std::istringstream wordStream(content);
            std::string word;
            while (wordStream >> word) {
                partialIndex[word][filepathStr]++;
            }
        }

        docCount += localDocCount;
    };

    std::vector<std::thread> threads;
    size_t filesPerThread = (filePaths.size() + numThreads - 1) / numThreads;

    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * filesPerThread;
        size_t end = std::min(start + filesPerThread, filePaths.size());
        threads.emplace_back(processFiles, t, start, end);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    totalDocuments = docCount.load();

    size_t estimatedTerms = 0;
    for (const auto& partialIndex : partialIndexes) {
        estimatedTerms += partialIndex.size();
    }
    invertedIndex.reserve(estimatedTerms);

    for (const auto& partialIndex : partialIndexes) {
        for (const auto& [term, posting] : partialIndex) {
            auto& mainPosting = invertedIndex[term];
            for (const auto& [doc, tf] : posting) {
                mainPosting[doc] += tf;
            }
        }
    }

    std::vector<std::pair<std::string, std::unordered_map<std::string, int>*>> termEntries;
    termEntries.reserve(invertedIndex.size());
    for (auto& [term, posting] : invertedIndex) {
        termEntries.emplace_back(term, &posting);
    }

    std::vector<std::unordered_map<std::string, std::unordered_map<std::string, double>>> partialDocTFIDF(numThreads);
    
    auto computeTFIDF = [&](size_t threadId, size_t start, size_t end) {
        auto& partialTFIDF = partialDocTFIDF[threadId];
        for (size_t i = start; i < end; ++i) {
            const auto& [term, posting] = termEntries[i];
            double idf = std::log((double)totalDocuments / posting->size());
            for (const auto& [doc, tf] : *posting) {
                partialTFIDF[doc][term] = tf * idf;
            }
        }
    };

    threads.clear();
    size_t termsPerThread = (termEntries.size() + numThreads - 1) / numThreads;

    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * termsPerThread;
        size_t end = std::min(start + termsPerThread, termEntries.size());
        threads.emplace_back(computeTFIDF, t, start, end);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    size_t estimatedDocs = 0;
    for (const auto& partialTFIDF : partialDocTFIDF) {
        estimatedDocs += partialTFIDF.size();
    }
    docTFIDF.reserve(estimatedDocs);
    
    for (const auto& partialTFIDF : partialDocTFIDF) {
        for (const auto& [doc, termScores] : partialTFIDF) {
            docTFIDF[doc].insert(termScores.begin(), termScores.end());
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