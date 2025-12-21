#include "preprocess.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>

std::string cleanText(const std::string& text) {
    std::string cleanedText;

    for(char c: text) {
        cleanedText.push_back(std::tolower(c));
    }

    auto kachra = std::remove_if(cleanedText.begin(), cleanedText.end(), [](char c) {
        if(!(std::isalnum(c) || std::isspace(c))) {
            return true;
        } else {
            return false;
        }
    });

    std::for_each(kachra, cleanedText.end(), [](char& c) {
        if (!std::isalnum(c) && !std::isspace(c)) {
            c = ' ';
        }
    });

    return cleanedText;
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

        std::string cleanedText = cleanText(buffer.str());

        std::ofstream outFile(entry.path(), std::ios::trunc);
        if (!outFile) {
            std::cerr << "Failed to open file for writing: " << entry.path() << std::endl;
            continue;
        }
        outFile<<cleanedText;
        outFile.close();
    }
}