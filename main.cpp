#include <iostream>
#include "preprocess/preprocess.h"
#include "tfidf/tfidf.h"

int main() {
    std::string dir = "20_newsgroups";

    std::cout << "Step 1: Preprocessing documents in \"" << dir << "\"..." << std::endl;
    cleanDocs(dir);
    std::cout << "Preprocessing done." << std::endl << std::endl;

    std::cout << "Step 2: Building TF-IDF index..." << std::endl;
    buildIndex(dir);
    std::cout << "Indexing completed. Total documents indexed: " << totalDocuments << std::endl << std::endl;

    std::string query;
    std::cout << "Step 3: Enter your query: ";
    std::getline(std::cin, query);
    std::cout << "Searching for query: \"" << query << "\"..." << std::endl;

    auto results = rankTFIDF(query, 20);

    if (results.empty()) {
        std::cout << "No matching documents found." << std::endl;
    } else {
        std::cout << "Top matching documents:" << std::endl;
        for (auto& [doc, score] : results) {
            std::cout << "Document: " << doc << " -> Score: " << score << std::endl;
        }
    }

    std::cout << "Search completed." << std::endl;
}
