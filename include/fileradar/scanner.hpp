#pragma once
#include "fileradar/types.hpp"
#include <filesystem>
namespace fileradar {
// Recursively scans regular files. Symbolic links are not followed.
ScanResult scan_directory(const std::filesystem::path& root);
} // namespace fileradar
