#include "preprocess.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <set>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

bool endsWith(const std::string& word, const std::string& suffix) {
    return word.length() >= suffix.length() && word.substr(word.length() - suffix.length()) == suffix;
}

std::string stemmer(const std::string& word) {
    if (word.length() <= 3) return word;

    if (endsWith(word, "ing") && word.length() > 4)
        return word.substr(0, word.length() - 3);

    if (endsWith(word, "ed") && word.length() > 3)
        return word.substr(0, word.length() - 2);

    if (endsWith(word, "es") && word.length() > 3)
        return word.substr(0, word.length() - 2);

    if (endsWith(word, "s") && word.length() > 2)
        return word.substr(0, word.length() - 1);

    return word;
}

bool isStopWord(const std::string& word) {
    static const std::set<std::string> stopWords = {
        "a","and","the","of","to","in","for","on","by",
        "with","is","it","this","that","as","be","are"
    };
    return (stopWords.count(word) > 0 || word.length() < 3 || word.length() > 10);
}

std::string cleaner(const std::string& text) {
    std::ostringstream out;
    std::string word;

    for (char c : text) {
        unsigned char uc = static_cast<unsigned char>(c);

        if (std::isalnum(uc)) {
            word.push_back(std::tolower(uc));
        } else {
            if (!word.empty()) {
                std::string stemmed = stemmer(word);
                if (!isStopWord(stemmed)) {
                    out << stemmed << ' ';
                }
                word.clear();
            }
        }
    }

    if (!word.empty()) {
        std::string stemmed = stemmer(word);
        if (!isStopWord(stemmed)) {
            out << stemmed << ' ';
        }
    }

    return out.str();
}

void cleanDocs(const std::string& directory) {
    std::vector<std::filesystem::path> filePaths;
    for(const auto& entry: std::filesystem::recursive_directory_iterator(directory)) {
        if(entry.is_regular_file()) {
            filePaths.push_back(entry.path());
        }
    }

    if (filePaths.empty()) {
        return;
    }

    size_t numThreads = std::max(1u, std::thread::hardware_concurrency());
    numThreads = std::min(numThreads, filePaths.size());

    std::mutex errorMutex;

    auto processFile = [&errorMutex](const std::filesystem::path& filePath) {
        std::ifstream inFile(filePath);
        if (!inFile) {
            std::lock_guard<std::mutex> lock(errorMutex);
            std::cerr << "Failed to open file for reading: " << filePath << std::endl;
            return;
        }
        std::ostringstream buffer;
        buffer << inFile.rdbuf();
        inFile.close();

        std::string finalCleaned = cleaner(buffer.str());

        std::ofstream outFile(filePath, std::ios::trunc);
        if (!outFile) {
            std::lock_guard<std::mutex> lock(errorMutex);
            std::cerr << "Failed to open file for writing: " << filePath << std::endl;
            return;
        }
        outFile << finalCleaned;
        outFile.close();
    };

    std::vector<std::thread> threads;
    size_t filesPerThread = (filePaths.size() + numThreads - 1) / numThreads;

    for (size_t t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            size_t start = t * filesPerThread;
            size_t end = std::min(start + filesPerThread, filePaths.size());
            for (size_t i = start; i < end; ++i) {
                processFile(filePaths[i]);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}