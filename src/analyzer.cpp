#include "fileradar/analyzer.hpp"
#include <algorithm>
#include <map>
#include <utility>
namespace fileradar {
std::vector<FileEntry> largest_files(const std::vector<FileEntry>& files, std::size_t limit) {
    auto sorted = files;
    std::stable_sort(sorted.begin(), sorted.end(), [](const FileEntry& a, const FileEntry& b) {
        if (a.size != b.size) return a.size > b.size;
        return a.path.string() < b.path.string();
    });
    if (sorted.size() > limit) sorted.resize(limit);
    return sorted;
}
std::vector<ExtensionStats> analyze_extensions(const std::vector<FileEntry>& files) {
    std::map<std::string, ExtensionStats> grouped;
    for (const auto& file : files) {
        auto& stats = grouped[file.extension];
        stats.extension = file.extension;
        ++stats.file_count;
        stats.total_bytes += file.size;
    }
    std::vector<ExtensionStats> result;
    for (auto& item : grouped) result.push_back(std::move(item.second));
    std::stable_sort(result.begin(), result.end(), [](const ExtensionStats& a, const ExtensionStats& b) {
        if (a.total_bytes != b.total_bytes) return a.total_bytes > b.total_bytes;
        return a.extension < b.extension;
    });
    return result;
}
} // namespace fileradar
