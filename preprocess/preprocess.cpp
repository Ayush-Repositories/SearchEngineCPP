#include "preprocess.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <set>

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

std::string cleanText(const std::string& text) {
    std::string cleaned;
    cleaned.reserve(text.size());

    for (char c : text) {
        unsigned char uc = static_cast<unsigned char>(c);

        if (std::isalnum(uc)) {
            cleaned.push_back(std::tolower(uc));
        } else {
            cleaned.push_back(' ');
        }
    }
    
    auto new_end = std::unique(cleaned.begin(), cleaned.end(),
        [](char a, char b) {
            return std::isspace(static_cast<unsigned char>(a)) &&
                   std::isspace(static_cast<unsigned char>(b));
        });

    cleaned.erase(new_end, cleaned.end());

    return cleaned;
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
    for(const auto& entry: std::filesystem::recursive_directory_iterator(directory)) {
        if(!entry.is_regular_file()) continue;

        std::ifstream inFile(entry.path());
        if (!inFile) {
            std::cerr << "Failed to open file for reading: " << entry.path() << std::endl;
            continue;
        }
        std::ostringstream buffer;
        buffer << inFile.rdbuf();
        inFile.close();

        std::string finalCleaned = cleaner(buffer.str());

        std::ofstream outFile(entry.path(), std::ios::trunc);
        if (!outFile) {
            std::cerr << "Failed to open file for writing: " << entry.path() << std::endl;
            continue;
        }
        outFile<<finalCleaned;
        outFile.close();
    }
}