#include "filesystem.h"
#include "util.h"


std::vector<char> Filesystem::read_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        print("Could not open file '%s'!", file_path);
    }

    size_t file_size = (size_t) file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}