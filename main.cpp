#include <iostream>
#include <chrono>
#include "preprocess/preprocess.h"
#include "tfidf/tfidf.h"

int main() {
    std::string dir = "20_newsgroups";

    std::cout << "Step 1: Preprocessing documents in \"" << dir << "\"..." << std::endl;
    auto start1 = std::chrono::high_resolution_clock::now();
    cleanDocs(dir);
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    std::cout << "Preprocessing done. Time taken: " << duration1.count() << " ms (" 
              << duration1.count() / 1000.0 << " seconds)" << std::endl << std::endl;

    std::cout << "Step 2: Building TF-IDF index..." << std::endl;
    auto start2 = std::chrono::high_resolution_clock::now();
    buildIndex(dir);
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    std::cout << "Indexing completed. Total documents indexed: " << totalDocuments 
              << ". Time taken: " << duration2.count() << " ms (" 
              << duration2.count() / 1000.0 << " seconds)" << std::endl << std::endl;

    std::string query;
    std::cout << "Step 3: Enter your query: ";
    std::getline(std::cin, query);
    std::cout << "Searching for query: \"" << query << "\"..." << std::endl;
    
    auto start3 = std::chrono::high_resolution_clock::now();
    auto results = rankTFIDF(query, 20);
    auto end3 = std::chrono::high_resolution_clock::now();
    auto duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start3);

    if (results.empty()) {
        std::cout << "No matching documents found." << std::endl;
    } else {
        std::cout << "Top matching documents:" << std::endl;
        for (auto& [doc, score] : results) {
            std::cout << "Document: " << doc << " -> Score: " << score << std::endl;
        }
    }
    
    std::cout << "Search completed. Time taken: " << duration3.count() << " ms (" 
              << duration3.count() / 1000.0 << " seconds)" << std::endl;
    
    auto totalTime = duration1 + duration2 + duration3;
    std::cout << std::endl << "\\/\\/\\/\\ Timing Summary \\/\\/\\/\\" << std::endl;
    std::cout << "Step 1 (Preprocessing): " << duration1.count() << " ms" << std::endl;
    std::cout << "Step 2 (Indexing): " << duration2.count() << " ms" << std::endl;
    std::cout << "Step 3 (Searching): " << duration3.count() << " ms" << std::endl;
    std::cout << "Total time: " << totalTime.count() << " ms (" 
              << totalTime.count() / 1000.0 << " seconds)" << std::endl;
}
