#include "fileradar/repair.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

namespace fileradar {
namespace {

bool starts_with(const std::vector<unsigned char>& data,
                 const std::vector<unsigned char>& signature) {
    return data.size() >= signature.size() &&
        std::equal(signature.begin(), signature.end(), data.begin());
}

bool ends_with(const std::vector<unsigned char>& data,
               const std::vector<unsigned char>& signature) {
    return data.size() >= signature.size() &&
        std::equal(signature.rbegin(), signature.rend(), data.rbegin());
}

std::vector<unsigned char> read_bytes(const std::filesystem::path& path,
                                      std::uintmax_t offset,
                                      std::size_t count) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    input.seekg(static_cast<std::streamoff>(offset));
    std::vector<unsigned char> bytes(count);
    input.read(reinterpret_cast<char*>(bytes.data()),
               static_cast<std::streamsize>(count));
    bytes.resize(static_cast<std::size_t>(input.gcount()));
    return bytes;
}

} // namespace

const char* health_label(FileHealth health) {
    switch (health) {
        case FileHealth::LooksValid: return "NO OBVIOUS DAMAGE";
        case FileHealth::Suspicious: return "SUSPICIOUS";
        case FileHealth::Unsupported: return "UNSUPPORTED FORMAT";
        case FileHealth::ReadError: return "READ ERROR";
    }
    return "UNKNOWN";
}

FileDiagnostic diagnose_file(const std::filesystem::path& path) {
    FileDiagnostic result;
    result.path = path;

    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error) {
        result.health = FileHealth::ReadError;
        result.details = "Path is not an accessible regular file.";
        return result;
    }

    const auto size = std::filesystem::file_size(path, error);
    if (error) {
        result.health = FileHealth::ReadError;
        result.details = "Could not read file metadata.";
        return result;
    }

    const auto head = read_bytes(path, 0, 16);
    if (head.empty() && size != 0) {
        result.health = FileHealth::ReadError;
        result.details = "Could not read file contents.";
        return result;
    }

    const std::vector<unsigned char> pdf{'%', 'P', 'D', 'F', '-'};
    const std::vector<unsigned char> png{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    const std::vector<unsigned char> jpg{0xFF, 0xD8, 0xFF};
    const std::vector<unsigned char> zip_local{'P', 'K', 0x03, 0x04};
    const std::vector<unsigned char> zip_empty{'P', 'K', 0x05, 0x06};
    const std::vector<unsigned char> zip_spanned{'P', 'K', 0x07, 0x08};

    if (starts_with(head, pdf)) {
        result.format = "PDF";
        const auto tail_size = static_cast<std::size_t>(std::min<std::uintmax_t>(size, 4096));
        const auto tail = read_bytes(path, size - tail_size, tail_size);
        const std::vector<unsigned char> eof{'%', '%', 'E', 'O', 'F'};
        result.health = ends_with(tail, eof) ||
            std::search(tail.begin(), tail.end(), eof.begin(), eof.end()) != tail.end()
            ? FileHealth::LooksValid : FileHealth::Suspicious;
        result.details = result.health == FileHealth::LooksValid
            ? "PDF header and EOF marker found; deeper validation is not performed."
            : "PDF header found, but the EOF marker is missing near the end.";
    } else if (starts_with(head, png)) {
        result.format = "PNG";
        const auto tail_size = static_cast<std::size_t>(std::min<std::uintmax_t>(size, 12));
        const auto tail = read_bytes(path, size - tail_size, tail_size);
        const std::vector<unsigned char> iend{0x00, 0x00, 0x00, 0x00, 'I', 'E', 'N', 'D'};
        result.health = size >= 20 &&
            std::search(tail.begin(), tail.end(), iend.begin(), iend.end()) != tail.end()
            ? FileHealth::LooksValid : FileHealth::Suspicious;
        result.details = result.health == FileHealth::LooksValid
            ? "PNG signature and IEND marker found; chunk CRCs are not checked."
            : "PNG signature found, but the file appears truncated or lacks its IEND marker.";
    } else if (starts_with(head, jpg)) {
        result.format = "JPEG";
        const auto tail_size = static_cast<std::size_t>(std::min<std::uintmax_t>(size, 2));
        const auto tail = read_bytes(path, size - tail_size, tail_size);
        const std::vector<unsigned char> eoi{0xFF, 0xD9};
        result.health = size >= 4 && ends_with(tail, eoi)
            ? FileHealth::LooksValid : FileHealth::Suspicious;
        result.details = result.health == FileHealth::LooksValid
            ? "JPEG start and end markers found; image decoding is not performed."
            : "JPEG start marker found, but the end marker is missing.";
    } else if (starts_with(head, zip_local) || starts_with(head, zip_empty) ||
               starts_with(head, zip_spanned)) {
        result.format = "ZIP";
        const auto tail_size = static_cast<std::size_t>(std::min<std::uintmax_t>(size, 65557));
        const auto tail = read_bytes(path, size - tail_size, tail_size);
        const std::vector<unsigned char> eocd{'P', 'K', 0x05, 0x06};
        result.health = std::search(tail.begin(), tail.end(), eocd.begin(), eocd.end()) != tail.end()
            ? FileHealth::LooksValid : FileHealth::Suspicious;
        result.details = result.health == FileHealth::LooksValid
            ? "ZIP end-of-central-directory signature found; entries are not fully tested."
            : "ZIP signature found, but the end-of-central-directory record is missing.";
    } else {
        result.format = "Unknown";
        result.health = FileHealth::Unsupported;
        result.details = "No structural checker is available for this file format yet.";
    }

    if (size == 0) {
        result.health = FileHealth::Suspicious;
        result.details = "File is empty (0 bytes).";
    }
    return result;
}

} // namespace fileradar
