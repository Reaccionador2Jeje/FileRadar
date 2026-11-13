#include "fileradar/gui.hpp"

#include "fileradar/analyzer.hpp"
#include "fileradar/duplicates.hpp"
#include "fileradar/reports.hpp"
#include "fileradar/scanner.hpp"

#include <filesystem>
#include <sstream>
#include <string>

namespace fileradar {
namespace {
wxString human_bytes(std::uintmax_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    if (unit == 0) return wxString::Format("%llu %s",
        static_cast<unsigned long long>(bytes), units[unit]);
    return wxString::Format("%.2f %s", value, units[unit]);
}
}

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "FileRadar Desktop 1.3.0",
              wxDefaultPosition, wxSize(900, 650)) {
    auto* panel = new wxPanel(this);
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* heading = new wxStaticText(panel, wxID_ANY, "FileRadar");
    heading->SetFont(wxFontInfo(22).Bold());
    root->Add(heading, 0, wxALL, 12);

    auto* subtitle = new wxStaticText(panel, wxID_ANY,
        "Storage analyzer  |  Read-only scanning  |  Windows, Linux and macOS");
    root->Add(subtitle, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* path_row = new wxBoxSizer(wxHORIZONTAL);
    directory_ = new wxTextCtrl(panel, wxID_ANY,
        wxString::FromUTF8(std::filesystem::current_path().string()));
    auto* browse = new wxButton(panel, wxID_ANY, "Browse...");
    path_row->Add(directory_, 1, wxRIGHT, 8);
    path_row->Add(browse, 0);
    root->Add(path_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* actions = new wxBoxSizer(wxHORIZONTAL);
    auto* scan = new wxButton(panel, wxID_ANY, "Analyze folder");
    largest_button_ = new wxButton(panel, wxID_ANY, "Largest files");
    duplicates_button_ = new wxButton(panel, wxID_ANY, "Find duplicates");
    export_button_ = new wxButton(panel, wxID_ANY, "Export JSON");
    largest_button_->Enable(false);
    duplicates_button_->Enable(false);
    export_button_->Enable(false);
    actions->Add(scan, 0, wxRIGHT, 8);
    actions->Add(largest_button_, 0, wxRIGHT, 8);
    actions->Add(duplicates_button_, 0, wxRIGHT, 8);
    actions->Add(export_button_, 0);
    root->Add(actions, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    output_ = new wxTextCtrl(panel, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxHSCROLL);
    output_->SetFont(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE));
    root->Add(output_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* footer = new wxStaticText(panel, wxID_ANY,
        "FileRadar never deletes or modifies scanned files. Symbolic links are skipped.");
    root->Add(footer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    panel->SetSizer(root);
    CreateStatusBar();
    SetStatusText("Ready");

    browse->Bind(wxEVT_BUTTON, &MainFrame::choose_directory, this);
    scan->Bind(wxEVT_BUTTON, &MainFrame::run_scan, this);
    largest_button_->Bind(wxEVT_BUTTON, &MainFrame::show_largest, this);
    duplicates_button_->Bind(wxEVT_BUTTON, &MainFrame::show_duplicates, this);
    export_button_->Bind(wxEVT_BUTTON, &MainFrame::export_report, this);

    append("Welcome to FileRadar Desktop. Choose a folder and start an analysis.\n");
    Centre();
}

void MainFrame::append(const wxString& text) {
    output_->AppendText(text);
}

void MainFrame::choose_directory(wxCommandEvent&) {
    wxDirDialog dialog(this, "Choose a folder to analyze",
                       directory_->GetValue(), wxDD_DEFAULT_STYLE);
    if (dialog.ShowModal() == wxID_OK) {
        directory_->SetValue(dialog.GetPath());
    }
}

void MainFrame::run_scan(wxCommandEvent&) {
    const std::filesystem::path path(directory_->GetValue().ToStdString());
    std::error_code ec;
    if (!std::filesystem::is_directory(path, ec) || ec) {
        wxMessageBox("Please choose an accessible folder.", "Invalid folder",
                     wxOK | wxICON_WARNING, this);
        return;
    }

    wxProgressDialog progress("FileRadar scan", "Preparing scan...",
        100, this, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_ELAPSED_TIME);
    SetStatusText("Scanning...");
    output_->Clear();
    append("Scanning: " + wxString::FromUTF8(path.string()) + "\n\n");

    last_result_ = scan_directory(path, [&progress](const ScanProgress& state) {
        const wxString message = wxString::Format(
            "Checked: %llu   Files: %llu   Folders: %llu   Data: %s   Errors: %llu",
            static_cast<unsigned long long>(state.entries_processed),
            static_cast<unsigned long long>(state.files_found),
            static_cast<unsigned long long>(state.directories_visited),
            human_bytes(state.bytes_found),
            static_cast<unsigned long long>(state.errors));
        progress.Pulse(message);
        wxYieldIfNeeded();
    });

    has_scan_ = last_result_.root_valid;
    largest_button_->Enable(has_scan_);
    duplicates_button_->Enable(has_scan_);
    export_button_->Enable(has_scan_);
    if (!has_scan_) {
        append("The folder could not be scanned. Check its permissions.\n");
        SetStatusText("Scan failed");
        return;
    }

    append("SCAN SUMMARY\n");
    append(wxString::Format("Files found: %llu\n",
        static_cast<unsigned long long>(last_result_.files.size())));
    append(wxString::Format("Folders visited: %llu\n",
        static_cast<unsigned long long>(last_result_.directories_visited)));
    append("Total size: " + human_bytes(last_result_.total_bytes) + "\n");
    append(wxString::Format("Skipped entries: %llu\n",
        static_cast<unsigned long long>(last_result_.entries_skipped)));
    append(wxString::Format("Errors: %llu\n\n",
        static_cast<unsigned long long>(last_result_.errors)));

    append("FILE TYPES\n");
    const auto extensions = analyze_extensions(last_result_.files);
    for (const auto& item : extensions) {
        append(wxString::Format("%s  |  %llu files  |  %s\n",
            wxString::FromUTF8(item.extension),
            static_cast<unsigned long long>(item.file_count),
            human_bytes(item.total_bytes)));
    }
    SetStatusText("Scan complete");
}

void MainFrame::show_largest(wxCommandEvent&) {
    if (!has_scan_) return;
    output_->Clear();
    append("LARGEST FILES\n\n");
    const auto files = largest_files(last_result_.files, 30);
    for (std::size_t i = 0; i < files.size(); ++i) {
        append(wxString::Format("%2llu. %10s  %s\n",
            static_cast<unsigned long long>(i + 1),
            human_bytes(files[i].size),
            wxString::FromUTF8(files[i].path.string())));
    }
    SetStatusText("Largest files");
}

void MainFrame::show_duplicates(wxCommandEvent&) {
    if (!has_scan_) return;
    output_->Clear();
    append("DUPLICATE FILES\n");
    const auto groups = find_duplicates(last_result_.files);
    if (groups.empty()) {
        append("\nNo duplicate files found.\n");
        SetStatusText("No duplicates");
        return;
    }
    for (std::size_t i = 0; i < groups.size(); ++i) {
        append(wxString::Format("\nGroup %llu\n",
            static_cast<unsigned long long>(i + 1)));
        for (const auto& file : groups[i]) {
            append(wxString::FromUTF8(file.path.string()) + "\n");
        }
    }
    SetStatusText(wxString::Format("%llu duplicate groups",
        static_cast<unsigned long long>(groups.size())));
}

void MainFrame::export_report(wxCommandEvent&) {
    if (!has_scan_) return;
    wxFileDialog dialog(this, "Save FileRadar report", "", "fileradar-report.json",
        "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) return;

    std::string error;
    if (!write_json_report(last_result_, dialog.GetPath().ToStdString(), error)) {
        wxMessageBox(wxString::FromUTF8(error), "Export failed",
                     wxOK | wxICON_ERROR, this);
        return;
    }
    SetStatusText("JSON report exported");
    wxMessageBox("Report saved successfully.", "FileRadar",
                 wxOK | wxICON_INFORMATION, this);
}

} // namespace fileradar
