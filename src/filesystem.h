#pragma once

#include <string>
#include <vector>
#include <fstream>

class Filesystem {
public:
    static std::vector<char> read_file(const std::string& file_path);
};