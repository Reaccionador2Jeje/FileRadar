#pragma once
#include "fileradar/types.hpp"
#include <vector>
namespace fileradar {
// Fast fingerprints narrow candidates; byte comparison confirms every duplicate.
std::vector<std::vector<FileEntry>> find_duplicates(const std::vector<FileEntry>& files);
} // namespace fileradar
