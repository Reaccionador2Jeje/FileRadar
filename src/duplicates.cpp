#include "fileradar/duplicates.hpp"
#include <array>
#include <fstream>
#include <map>
#include <utility>
namespace fileradar {
namespace {
bool fingerprint_file(const std::filesystem::path& path, std::uint64_t& hash) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    hash = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;
    std::array<char, 65536> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        for (std::streamsize i = 0; i < count; ++i) {
            hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(i)]);
            hash *= prime;
        }
    }
    return input.eof();
}
bool files_equal(const std::filesystem::path& left, const std::filesystem::path& right) {
    std::ifstream a(left, std::ios::binary), b(right, std::ios::binary);
    if (!a || !b) return false;
    std::array<char, 65536> x{}, y{};
    while (a && b) {
        a.read(x.data(), static_cast<std::streamsize>(x.size()));
        b.read(y.data(), static_cast<std::streamsize>(y.size()));
        const auto nx = a.gcount(), ny = b.gcount();
        if (nx != ny) return false;
        for (std::streamsize i = 0; i < nx; ++i)
            if (x[static_cast<std::size_t>(i)] != y[static_cast<std::size_t>(i)]) return false;
    }
    return a.eof() && b.eof();
}
}
std::vector<std::vector<FileEntry>> find_duplicates(const std::vector<FileEntry>& files) {
    using Key = std::pair<std::uintmax_t, std::uint64_t>;
    std::map<Key, std::vector<FileEntry>> candidates;
    for (const auto& file : files) {
        std::uint64_t hash = 0;
        if (fingerprint_file(file.path, hash)) candidates[{file.size, hash}].push_back(file);
    }
    std::vector<std::vector<FileEntry>> duplicates;
    for (const auto& candidate : candidates) {
        if (candidate.second.size() < 2) continue;
        std::vector<std::vector<FileEntry>> confirmed;
        for (const auto& file : candidate.second) {
            bool placed = false;
            for (auto& group : confirmed) {
                if (files_equal(group.front().path, file.path)) {
                    group.push_back(file);
                    placed = true;
                    break;
                }
            }
            if (!placed) confirmed.push_back({file});
        }
        for (auto& group : confirmed)
            if (group.size() > 1) duplicates.push_back(std::move(group));
    }
    return duplicates;
}
} // namespace fileradar
