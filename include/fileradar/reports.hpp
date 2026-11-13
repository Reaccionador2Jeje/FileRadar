#pragma once
#include "fileradar/types.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>
namespace fileradar {
std::string make_json_report(const ScanResult& result);
bool write_json_report(const ScanResult& result, const std::filesystem::path& destination, std::string& error);
void print_scan_summary(const ScanResult& result, std::ostream& output);
} // namespace fileradar
