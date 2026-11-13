#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fileradar {
struct FileEntry {
    std::filesystem::path path;
    std::uintmax_t size = 0;
    std::string extension;
};
struct ScanResult {
    std::filesystem::path root;
    std::vector<FileEntry> files;
    std::uintmax_t total_bytes = 0;
    std::size_t directories_visited = 0;
    std::size_t entries_skipped = 0;
    std::size_t errors = 0;
    bool root_valid = false;
};
struct ExtensionStats {
    std::string extension;
    std::size_t file_count = 0;
    std::uintmax_t total_bytes = 0;
};
} // namespace fileradar
