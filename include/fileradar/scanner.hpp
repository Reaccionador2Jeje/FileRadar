#pragma once
#include "fileradar/types.hpp"
#include <filesystem>
#include <functional>
namespace fileradar {
// Recursively scans regular files. Symbolic links are not followed.
ScanResult scan_directory(
    const std::filesystem::path& root,
    const std::function<void(const ScanProgress&)>& on_progress = {});
} // namespace fileradar
