#pragma once
#include "fileradar/types.hpp"
#include <cstddef>
#include <vector>
namespace fileradar {
std::vector<FileEntry> largest_files(const std::vector<FileEntry>& files, std::size_t limit = 20);
std::vector<ExtensionStats> analyze_extensions(const std::vector<FileEntry>& files);
} // namespace fileradar
