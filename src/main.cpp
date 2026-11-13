#include "fileradar/analyzer.hpp"
#include "fileradar/duplicates.hpp"
#include "fileradar/reports.hpp"
#include "fileradar/repair.hpp"
#include "fileradar/scanner.hpp"
#include "fileradar/tui.hpp"

#include <filesystem>
#include <iostream>
#include <limits>
#include <string>

namespace {
void print_help() {
    std::cout
        << "FileRadar 1.3.0 - storage analyzer\n\n"
        << "Usage:\n"
        << "  fileradar [tui]\n"
        << "  fileradar scan <directory>\n"
        << "  fileradar large <directory> [limit]\n"
        << "  fileradar duplicates <directory>\n"
        << "  fileradar report <directory> [output.json]\n"
        << "  fileradar repair diagnose <file>\n"
        << "  fileradar repair scan <directory>\n"
        << "  fileradar --help\n"
        << "  fileradar --version\n\n"
        << "Commands:\n"
        << "  scan        Summarize files and storage usage.\n"
        << "  large       List the largest files (default: 20).\n"
        << "  duplicates  Find files with identical contents.\n"
        << "  report      Generate a JSON analysis report.\n"
        << "  repair      Run read-only structural file diagnostics.\n\n"
        << "Safety:\n"
        << "  All current commands are read-only; files are never modified.\n"
        << "  Symbolic links are not followed.\n";
}

bool parse_limit(const char* text, std::size_t& value) {
    try {
        const std::string input(text);
        if (input.empty() || input.front() == '-') return false;
        std::size_t used = 0;
        const auto parsed = std::stoull(input, &used);
        if (used != input.size() || parsed > std::numeric_limits<std::size_t>::max()) return false;
        value = static_cast<std::size_t>(parsed);
        return true;
    } catch (...) { return false; }
}

int run_command(const std::string& command, const std::string& directory, int argc, char* argv[]) {
    const auto result = fileradar::scan_directory(directory);
    if (!result.root_valid) {
        std::cerr << "Error: not an accessible directory: " << directory << "\n";
        return 2;
    }
    if (command == "scan") {
        fileradar::print_scan_summary(result, std::cout);
        return 0;
    }
    if (command == "large") {
        std::size_t limit = 20;
        if (argc >= 4 && !parse_limit(argv[3], limit)) {
            std::cerr << "Error: limit must be a non-negative integer.\n";
            return 1;
        }
        const auto files = fileradar::largest_files(result.files, limit);
        std::cout << "Largest files (" << files.size() << " shown):\n";
        for (const auto& file : files)
            std::cout << file.size << " bytes\t" << file.path.string() << "\n";
        return 0;
    }
    if (command == "duplicates") {
        const auto groups = fileradar::find_duplicates(result.files);
        if (groups.empty()) { std::cout << "No duplicate files found.\n"; return 0; }
        std::size_t count = 0;
        for (std::size_t i = 0; i < groups.size(); ++i) {
            std::cout << "\nGroup " << (i + 1) << ":\n";
            for (const auto& file : groups[i]) {
                std::cout << "  " << file.path.string() << "\n";
                ++count;
            }
        }
        std::cout << "\nDuplicate groups: " << groups.size()
                  << "\nFiles in duplicate groups: " << count << "\n";
        return 0;
    }
    if (command == "report") {
        if (argc >= 4) {
            std::string error;
            if (!fileradar::write_json_report(result, argv[3], error)) {
                std::cerr << "Error: " << error << "\n";
                return 2;
            }
            std::cout << "JSON report written to: " << argv[3] << "\n";
        } else {
            std::cout << fileradar::make_json_report(result);
        }
        return 0;
    }
    return 1;
}
} // namespace

int run_repair(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: fileradar repair <diagnose|scan> <file-or-directory>\n";
        return 1;
    }

    const std::string action = argv[2];
    const std::filesystem::path target(argv[3]);

    if (action == "diagnose") {
        const auto item = fileradar::diagnose_file(target);
        std::cout << "File: " << item.path.string()
                  << "\nFormat: " << item.format
                  << "\nStatus: " << fileradar::health_label(item.health)
                  << "\nDetails: " << item.details << "\n";
        return item.health == fileradar::FileHealth::ReadError ? 2 : 0;
    }

    if (action == "scan") {
        const auto scan = fileradar::scan_directory(target);
        if (!scan.root_valid) {
            std::cerr << "Error: directory is not accessible.\n";
            return 2;
        }
        std::size_t checked = 0, suspicious = 0, unsupported = 0, errors = 0;
        for (const auto& file : scan.files) {
            const auto item = fileradar::diagnose_file(file.path);
            ++checked;
            if (item.health == fileradar::FileHealth::Suspicious) ++suspicious;
            else if (item.health == fileradar::FileHealth::Unsupported) ++unsupported;
            else if (item.health == fileradar::FileHealth::ReadError) ++errors;

            if (item.health != fileradar::FileHealth::LooksValid) {
                std::cout << fileradar::health_label(item.health) << " | "
                          << item.format << " | " << item.path.string() << "\n"
                          << "  " << item.details << "\n";
            }
        }
        std::cout << "\nChecked: " << checked << " | Suspicious: " << suspicious
                  << " | Unsupported: " << unsupported << " | Read errors: " << errors << "\n";
        std::cout << "Structural screening only: this does not prove a file is healthy or damaged.\n";
        return 0;
    }

    std::cerr << "Unknown repair action. Use diagnose or scan.\n";
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) return fileradar::run_tui();

    const std::string command = argv[1];
    if (command == "tui") return fileradar::run_tui();
    if (command == "repair") return run_repair(argc, argv);
    if (command == "--help" || command == "-h" || command == "help") {
        print_help();
        return 0;
    }
    if (command == "--version" || command == "-v") {
        std::cout << "FileRadar 1.3.0\n";
        return 0;
    }

    if (argc < 3) {
        std::cerr << "Error: a directory path is required.\n\n";
        print_help();
        return 1;
    }
    if (command != "scan" && command != "large" &&
        command != "duplicates" && command != "report") {
        std::cerr << "Error: unknown command: " << command << "\n\n";
        print_help();
        return 1;
    }
    if (argc > 4 || (command != "large" && command != "report" && argc > 3)) {
        std::cerr << "Error: too many arguments.\n";
        return 1;
    }
    return run_command(command, argv[2], argc, argv);
}
