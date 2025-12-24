#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <filesystem>
#include <fstream>
#include <iostream>

std::string cleaner(const std::string& text);
void cleanDocs(const std::string& directory);

#endif