#include "loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<Document> loadDocs(const std::string &directory) {
    std::vector<Document> docs;

    for(const auto& entry : fs::recursive_directory_iterator(directory)) {
        if(!entry.is_regular_file()) continue;

        std::string filepath = entry.path().string();
        size_t lastSlash = filepath.find_last_of("/\\");
        std::string filename = filepath.substr(lastSlash + 1);

        std::ifstream file(entry.path());
        if(!file.is_open()) {
            std::cerr<<"Could not open "<<entry.path()<<std::endl; // cerr because it is unbuffered, i.e. immediate output
            continue;
        }

        std::stringstream buffer;
        buffer<<file.rdbuf();

        docs.push_back({ filepath, filename, buffer.str() });
    }

    return docs;
}