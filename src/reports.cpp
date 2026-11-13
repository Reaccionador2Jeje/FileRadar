#include "fileradar/reports.hpp"
#include "fileradar/analyzer.hpp"
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
namespace fileradar {
namespace {
std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20) out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                               << static_cast<unsigned int>(ch) << std::dec << std::setfill(' ');
            else out << static_cast<char>(ch);
        }
    }
    return out.str();
}
}
std::string make_json_report(const ScanResult& result) {
    const auto extensions = analyze_extensions(result.files);
    const auto largest = largest_files(result.files, 20);
    std::ostringstream out;
    out << "{\n  \"application\": \"FileRadar\",\n"
        << "  \"version\": \"1.0.0\",\n"
        << "  \"root\": \"" << json_escape(result.root.string()) << "\",\n"
        << "  \"root_valid\": " << (result.root_valid ? "true" : "false") << ",\n"
        << "  \"summary\": {\n"
        << "    \"file_count\": " << result.files.size() << ",\n"
        << "    \"total_bytes\": " << result.total_bytes << ",\n"
        << "    \"directories_visited\": " << result.directories_visited << ",\n"
        << "    \"entries_skipped\": " << result.entries_skipped << ",\n"
        << "    \"errors\": " << result.errors << "\n  },\n"
        << "  \"extensions\": [\n";
    for (std::size_t i = 0; i < extensions.size(); ++i) {
        const auto& item = extensions[i];
        out << "    {\"extension\": \"" << json_escape(item.extension)
            << "\", \"file_count\": " << item.file_count
            << ", \"total_bytes\": " << item.total_bytes << "}";
        out << (i + 1 == extensions.size() ? "\n" : ",\n");
    }
    out << "  ],\n  \"largest_files\": [\n";
    for (std::size_t i = 0; i < largest.size(); ++i) {
        const auto& file = largest[i];
        out << "    {\"path\": \"" << json_escape(file.path.string())
            << "\", \"size_bytes\": " << file.size << "}";
        out << (i + 1 == largest.size() ? "\n" : ",\n");
    }
    out << "  ]\n}\n";
    return out.str();
}
bool write_json_report(const ScanResult& result, const std::filesystem::path& destination, std::string& error) {
    std::ofstream output(destination, std::ios::binary);
    if (!output) { error = "Could not open report destination for writing."; return false; }
    output << make_json_report(result);
    if (!output) { error = "An error occurred while writing the report."; return false; }
    return true;
}
void print_scan_summary(const ScanResult& result, std::ostream& output) {
    output << "FileRadar 1.0.0\n"
           << "Root: " << result.root.string() << "\n"
           << "Files: " << result.files.size() << "\n"
           << "Total size: " << result.total_bytes << " bytes\n"
           << "Directories visited: " << result.directories_visited << "\n"
           << "Skipped entries: " << result.entries_skipped << "\n"
           << "Scan errors: " << result.errors << "\n";
}
} // namespace fileradar
