#ifndef LOADER_H
#define LOADER_H

#include<string>
#include<vector>

struct Document {
    std::string path;
    std::string filename;
};

extern size_t totalDocuments;

std::vector<Document> loadDocs(const std::string &directory);

#endif