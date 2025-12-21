#include <iostream>
#include "loader.h"

int main() {
    std::string dir = "20_newsgroups";

    std::vector<Document> docs = loadDocs(dir);

    for(const auto& doc: docs) {
        std::cout<<"Path: "<<doc.path<<std::endl;
    }
}