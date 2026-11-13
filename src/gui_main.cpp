#include <wx/wx.h>
#include "fileradar/gui.hpp"

class FileRadarApp final : public wxApp {
public:
    bool OnInit() override {
        auto* frame = new fileradar::MainFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(FileRadarApp);
