#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <filesystem>
#include <fstream>
#include <iostream>

std::string cleanText(const std::string& text);
void cleanDocs(const std::string& directory);

#endif