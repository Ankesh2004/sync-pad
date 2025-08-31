#include "common.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>

int main(int argc, char** argv) {
    Options opt = parse_args(argc, argv);

    if (opt.help || opt.role.empty() || opt.host.empty()) {
        print_help(argv[0]);
        return 0;
    }

    // Simple FLTK window for now
    Fl_Window win(400, 200, "SyncPad GUI");
    std::string label = "Role: " + opt.role + "\nPeer: " 
                        + opt.host + ":" + std::to_string(opt.port);
    Fl_Box box(20, 40, 360, 100, label.c_str());
    win.end();
    win.show(argc, argv);

    return Fl::run();
}
