#pragma once

#include "fileradar/types.hpp"
#include <wx/wx.h>

namespace fileradar {

class MainFrame final : public wxFrame {
public:
    MainFrame();

private:
    wxTextCtrl* directory_ = nullptr;
    wxTextCtrl* output_ = nullptr;
    wxButton* largest_button_ = nullptr;
    wxButton* duplicates_button_ = nullptr;
    wxButton* export_button_ = nullptr;
    ScanResult last_result_;
    bool has_scan_ = false;

    void choose_directory(wxCommandEvent& event);
    void run_scan(wxCommandEvent& event);
    void show_largest(wxCommandEvent& event);
    void show_duplicates(wxCommandEvent& event);
    void export_report(wxCommandEvent& event);
    void append(const wxString& text);
};

} // namespace fileradar
