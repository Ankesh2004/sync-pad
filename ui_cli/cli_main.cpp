#include "common.hpp"

int main(int argc, char** argv) {
    Options opt = parse_args(argc, argv);

    if (opt.help || opt.role.empty() || opt.host.empty()) {
        print_help(argv[0]);
        return 0;
    }

    std::cout << "[CLI] Role: " << opt.role 
              << ", Peer: " << opt.host 
              << ":" << opt.port << "\n";

    // Placeholder: here will go terminal editor + networking
    return 0;
}
