#include "fileradar/tui.hpp"
#include "fileradar/analyzer.hpp"
#include "fileradar/duplicates.hpp"
#include "fileradar/reports.hpp"
#include "fileradar/repair.hpp"
#include "fileradar/scanner.hpp"

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <limits>
#include <string>

namespace fileradar {
namespace {
constexpr const char* reset = "\033[0m";
constexpr const char* cyan = "\033[36m";
constexpr const char* green = "\033[32m";
constexpr const char* yellow = "\033[33m";
constexpr const char* red = "\033[31m";
constexpr const char* bold = "\033[1m";

std::string format_bytes(std::uintmax_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    char buffer[64];
    if (unit == 0) {
        std::snprintf(buffer, sizeof(buffer), "%llu %s",
                      static_cast<unsigned long long>(bytes), units[unit]);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[unit]);
    }
    return buffer;
}

void wait_for_enter() {
    std::cout << "\nPresiona ENTER para volver al menu...";
    std::string ignored;
    std::getline(std::cin, ignored);
}

void draw_header(const std::string& directory) {
    std::cout << "\033[2J\033[H";
    std::cout << cyan << bold
              << "  ______ _ _      _____           _\n"
              << " |  ____(_) |    |  __ \\         | |\n"
              << " | |__   _| | ___| |__) |__ _  __| | __ _ _ __\n"
              << " |  __| | | |/ _ \\  _  // _` |/ _` |/ _` | '__|\n"
              << " | |    | | |  __/ | \\ \\ (_| | (_| | (_| | |\n"
              << " |_|    |_|_|\\___|_|  \\_\\__,_|\\__,_|\\__,_|_|\n"
              << reset;
    std::cout << "  " << green << "FileRadar TUI" << reset
              << "  |  Storage analyzer\n";
    std::cout << "  Carpeta: " << yellow << directory << reset << "\n";
    std::cout << "  --------------------------------------------------\n";
}

bool scan_for_tui(const std::string& directory, ScanResult& result) {
    std::cout << yellow << "\n[>] Escaneando carpeta...\n" << reset;
    result = scan_directory(directory, [](const ScanProgress& progress) {
        static const char spinner[] = {'|', '/', '-', '\\'};
        const char frame = spinner[progress.entries_processed / 32 % 4];
        std::cout << "\r  " << cyan << "[" << frame << "]" << reset
                  << " Revisados: " << progress.entries_processed
                  << " | Archivos: " << progress.files_found
                  << " | Carpetas: " << progress.directories_visited
                  << " | Datos: " << format_bytes(progress.bytes_found)
                  << " | Errores: " << progress.errors
                  << "   " << std::flush;
    });
    std::cout << "\n";
    if (!result.root_valid) {
        std::cout << red << "No se pudo abrir esa carpeta. Revisa la ruta y permisos.\n" << reset;
        return false;
    }
    std::cout << green << "[OK] Escaneo terminado.\n" << reset;
    return true;
}

void show_scan(const ScanResult& result) {
    std::cout << "\n" << bold << "RESUMEN DEL ESCANEO" << reset << "\n";
    std::cout << "Archivos:          " << result.files.size() << "\n";
    std::cout << "Carpetas visitadas:" << result.directories_visited << "\n";
    std::cout << "Espacio ocupado:   " << format_bytes(result.total_bytes) << "\n";
    std::cout << "Elementos omitidos:" << result.entries_skipped << "\n";
    std::cout << "Errores:           " << result.errors << "\n";

    const auto extensions = analyze_extensions(result.files);
    if (!extensions.empty()) {
        std::cout << "\n" << bold << "TIPOS DE ARCHIVO (por espacio)" << reset << "\n";
        const std::size_t limit = extensions.size() < 8 ? extensions.size() : 8;
        for (std::size_t i = 0; i < limit; ++i) {
            std::cout << "  " << extensions[i].extension << "  "
                      << extensions[i].file_count << " archivos  "
                      << format_bytes(extensions[i].total_bytes) << "\n";
        }
    }
}

void show_largest(const ScanResult& result) {
    const auto files = largest_files(result.files, 15);
    std::cout << "\n" << bold << "15 ARCHIVOS MAS GRANDES" << reset << "\n";
    if (files.empty()) {
        std::cout << "No se encontraron archivos.\n";
        return;
    }
    for (std::size_t i = 0; i < files.size(); ++i) {
        std::cout << (i + 1) << ". " << format_bytes(files[i].size)
                  << "  " << files[i].path.string() << "\n";
    }
}

void show_duplicates(const ScanResult& result) {
    std::cout << "\n" << bold << "ARCHIVOS DUPLICADOS" << reset << "\n";
    const auto groups = find_duplicates(result.files);
    if (groups.empty()) {
        std::cout << green << "No se encontraron duplicados.\n" << reset;
        return;
    }
    std::size_t total = 0;
    for (std::size_t i = 0; i < groups.size(); ++i) {
        std::cout << "\nGrupo " << (i + 1) << ":\n";
        for (const auto& file : groups[i]) {
            std::cout << "  " << file.path.string() << "\n";
            ++total;
        }
    }
    std::cout << "\nGrupos: " << groups.size()
              << " | Archivos implicados: " << total << "\n";
}

void save_report(const ScanResult& result) {
    std::cout << "Ruta del informe JSON (ej. report.json): ";
    std::string output;
    std::getline(std::cin, output);
    if (output.empty()) {
        std::cout << "Operacion cancelada: no se indico una ruta.\n";
        return;
    }
    std::string error;
    if (!write_json_report(result, output, error)) {
        std::cout << red << "No se pudo guardar: " << error << reset << "\n";
        return;
    }
    std::cout << green << "Informe guardado en: " << output << reset << "\n";
}

void diagnose_one_file() {
    std::cout << "Ruta del archivo: ";
    std::string path;
    std::getline(std::cin, path);
    if (path.empty()) {
        std::cout << yellow << "Diagnostico cancelado.\n" << reset;
        return;
    }

    const auto item = diagnose_file(path);
    std::cout << "\n" << bold << "DIAGNOSTICO DE ARCHIVO" << reset << "\n";
    std::cout << "Archivo: " << item.path.string() << "\n";
    std::cout << "Formato: " << item.format << "\n";
    std::cout << "Estado:  ";
    if (item.health == FileHealth::LooksValid)
        std::cout << green;
    else if (item.health == FileHealth::Suspicious || item.health == FileHealth::ReadError)
        std::cout << red;
    else
        std::cout << yellow;
    std::cout << health_label(item.health) << reset << "\n";
    std::cout << "Detalle: " << item.details << "\n";
    std::cout << yellow
              << "Nota: la revision estructural no garantiza que el archivo este completamente sano.\n"
              << reset;
}

void diagnose_directory(const std::string& directory) {
    std::cout << yellow << "\n[>] Revisando estructura de archivos...\n" << reset;
    ScanResult scan;
    if (!scan_for_tui(directory, scan)) return;

    std::size_t checked = 0;
    std::size_t valid = 0;
    std::size_t suspicious = 0;
    std::size_t unsupported = 0;
    std::size_t errors = 0;

    std::cout << "\n" << bold << "RESULTADOS DEL DIAGNOSTICO" << reset << "\n";
    for (const auto& file : scan.files) {
        const auto item = diagnose_file(file.path);
        ++checked;
        switch (item.health) {
            case FileHealth::LooksValid: ++valid; break;
            case FileHealth::Suspicious: ++suspicious; break;
            case FileHealth::Unsupported: ++unsupported; break;
            case FileHealth::ReadError: ++errors; break;
        }

        if (item.health != FileHealth::LooksValid) {
            const char* color = item.health == FileHealth::Suspicious ||
                                item.health == FileHealth::ReadError ? red : yellow;
            std::cout << color << "[" << health_label(item.health) << "] "
                      << item.format << reset << " | " << item.path.string() << "\n"
                      << "  " << item.details << "\n";
        }
    }

    std::cout << "\n" << bold << "RESUMEN DE DIAGNOSTICO" << reset << "\n"
              << "Revisados:    " << checked << "\n"
              << green << "Validos:      " << valid << reset << "\n"
              << red << "Sospechosos:  " << suspicious << reset << "\n"
              << yellow << "No soportados:" << unsupported << reset << "\n"
              << red << "Errores:      " << errors << reset << "\n";
    std::cout << yellow
              << "Aviso: FileRadar realiza comprobaciones estructurales basicas; "
              << "no modifica ni repara archivos.\n"
              << reset;
}
} // namespace

int run_tui() {
    std::string directory = ".";
    while (true) {
        draw_header(directory);
        std::cout
            << "\n  " << bold << "MENU PRINCIPAL" << reset << "\n"
            << "  [1] Analizar almacenamiento\n"
            << "  [2] Ver archivos mas grandes\n"
            << "  [3] Buscar duplicados\n"
            << "  [4] Exportar informe JSON\n"
            << "  [5] Diagnosticar un archivo\n"
            << "  [6] Buscar archivos sospechosos en la carpeta\n"
            << "  [7] Cambiar carpeta\n"
            << "  [0] Salir\n"
            << "\n  Selecciona una opcion: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) return 0;
        if (choice == "0" || choice == "q" || choice == "Q") {
            std::cout << green << "\nFileRadar TUI cerrado. Hasta luego.\n" << reset;
            return 0;
        }
        if (choice == "7") {
            std::cout << "Nueva ruta: ";
            std::string next;
            std::getline(std::cin, next);
            if (!next.empty()) directory = next;
            continue;
        }
        if (choice == "5") {
            diagnose_one_file();
            wait_for_enter();
            continue;
        }
        if (choice == "6") {
            diagnose_directory(directory);
            wait_for_enter();
            continue;
        }
        if (choice != "1" && choice != "2" && choice != "3" && choice != "4") {
            std::cout << red << "\nOpcion no valida.\n" << reset;
            wait_for_enter();
            continue;
        }

        ScanResult result;
        if (scan_for_tui(directory, result)) {
            if (choice == "1") show_scan(result);
            else if (choice == "2") show_largest(result);
            else if (choice == "3") show_duplicates(result);
            else save_report(result);
        }
        wait_for_enter();
    }
}
} // namespace fileradar
