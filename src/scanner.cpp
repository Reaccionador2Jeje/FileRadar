#include "fileradar/scanner.hpp"
#include <algorithm>
#include <cctype>
#include <system_error>
#include <utility>

namespace fileradar {
namespace {
std::string normalized_extension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return extension.empty() ? "(no extension)" : extension;
}
}
ScanResult scan_directory(const std::filesystem::path& root,\n                          const std::function<void(const ScanProgress&)>& on_progress) {
    ScanResult result;
    std::error_code error;
    result.root = std::filesystem::absolute(root, error);
    if (error) { result.root = root; error.clear(); }
    if (!std::filesystem::is_directory(root, error) || error) {
        result.errors = 1;
        return result;
    }
    result.root_valid = true;
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    std::filesystem::recursive_directory_iterator it(root, options, error);
    const std::filesystem::recursive_directory_iterator end;
    if (error) { ++result.errors; error.clear(); }
    ScanProgress progress;\n    std::size_t last_notified = 0;\n    while (it != end) {
        const auto entry = *it;
        std::error_code status_error;
        const auto status = entry.symlink_status(status_error);
        if (status_error) {
            ++result.errors;
        } else if (std::filesystem::is_symlink(status)) {
            // Do not inspect or follow symbolic links.
            ++result.entries_skipped;
        } else if (std::filesystem::is_directory(status)) {
            ++result.directories_visited;
        } else if (std::filesystem::is_regular_file(status)) {
            const auto size = entry.file_size(status_error);
            if (status_error) {
                ++result.errors;
            } else {
                result.files.push_back({entry.path(), size, normalized_extension(entry.path())});
                result.total_bytes += size;
            }
        } else {
            ++result.entries_skipped;
        }
        it.increment(error);
        if (error) { ++result.errors; error.clear(); }
    }
    return result;
}
} // namespace fileradar
