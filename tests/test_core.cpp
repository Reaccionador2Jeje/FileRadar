#include "fileradar/analyzer.hpp"
#include "fileradar/duplicates.hpp"
#include "fileradar/scanner.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
bool write_file(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream output(path, std::ios::binary);
    output << contents;
    return static_cast<bool>(output);
}
}
int main() {
    namespace fs = std::filesystem;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path root = fs::temp_directory_path() / ("fileradar-test-" + std::to_string(stamp));
    try {
        fs::create_directories(root / "nested");
        if (!write_file(root / "alpha.txt", "same contents") ||
            !write_file(root / "nested" / "copy.txt", "same contents") ||
            !write_file(root / "different.bin", "different"))
            throw std::runtime_error("could not create test files");
        const auto scan = fileradar::scan_directory(root);
        if (!scan.root_valid || scan.files.size() != 3 || scan.total_bytes == 0)
            throw std::runtime_error("directory scan returned unexpected results");
        const auto duplicates = fileradar::find_duplicates(scan.files);
        if (duplicates.size() != 1 || duplicates.front().size() != 2)
            throw std::runtime_error("duplicate detection failed");
        const auto largest = fileradar::largest_files(scan.files, 1);
        if (largest.size() != 1 || largest.front().size != 13)
            throw std::runtime_error("largest-file analysis failed");
        const auto extensions = fileradar::analyze_extensions(scan.files);
        if (extensions.size() != 2)
            throw std::runtime_error("extension analysis failed");
        fs::remove_all(root);
        std::cout << "All FileRadar core tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::error_code ignored;
        fs::remove_all(root, ignored);
        std::cerr << "Test failure: " << error.what() << "\n";
        return 1;
    }
}
