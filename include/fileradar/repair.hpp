#pragma once

#include <filesystem>
#include <string>

namespace fileradar {

enum class FileHealth {
    LooksValid,
    Suspicious,
    Unsupported,
    ReadError
};

struct FileDiagnostic {
    std::filesystem::path path;
    std::string format;
    FileHealth health = FileHealth::Unsupported;
    std::string details;
};

// Performs conservative, read-only structural checks for common file formats.
FileDiagnostic diagnose_file(const std::filesystem::path& path);
const char* health_label(FileHealth health);

} // namespace fileradar
