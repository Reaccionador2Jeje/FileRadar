#include "fileradar/scanner.hpp"
#include <algorithm>
#include <cctype>
#include <system_error>

namespace fileradar {
namespace {
std::string normalized_extension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return extension.empty() ? "(no extension)" : extension;
}
}

ScanResult scan_directory(
    const std::filesystem::path& root,
    const std::function<void(const ScanProgress&)>& on_progress) {
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

    ScanProgress progress;
    progress.errors = result.errors;
    std::size_t last_notified = 0;
    bool notified = false;

    while (it != end) {
        const auto entry = *it;
        std::error_code status_error;
        const auto status = entry.symlink_status(status_error);

        if (status_error) {
            ++result.errors;
            ++progress.errors;
        } else if (std::filesystem::is_symlink(status)) {
            ++result.entries_skipped;
            ++progress.entries_skipped;
        } else if (std::filesystem::is_directory(status)) {
            ++result.directories_visited;
            ++progress.directories_visited;
        } else if (std::filesystem::is_regular_file(status)) {
            const auto size = entry.file_size(status_error);
            if (status_error) {
                ++result.errors;
                ++progress.errors;
            } else {
                result.files.push_back({entry.path(), size, normalized_extension(entry.path())});
                result.total_bytes += size;
                ++progress.files_found;
                progress.bytes_found += size;
            }
        } else {
            ++result.entries_skipped;
            ++progress.entries_skipped;
        }

        ++progress.entries_processed;
        it.increment(error);
        if (error) {
            ++result.errors;
            ++progress.errors;
            error.clear();
        }

        if (on_progress && progress.entries_processed % 32 == 0) {
            on_progress(progress);
            last_notified = progress.entries_processed;
            notified = true;
        }
    }

    if (on_progress && (!notified || progress.entries_processed != last_notified)) {
        on_progress(progress);
    }
    return result;
}
} // namespace fileradar
