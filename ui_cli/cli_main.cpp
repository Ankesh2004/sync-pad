#include "../core/transport.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>

#include "../core/common.hpp" // reuse parsing for role/host/port or create small arg parse

void print_help(const char* prog) {
    std::cout << "Usage:\n"
              << prog << " --listen <port> [--peer <host>:<port>]\n"
              << "or\n"
              << prog << " --peer <host>:<port> [--listen <port>]\n";
}

int main(int argc, char** argv) {
    int listen_port = 0;
    std::string peer_host;
    int peer_port = 0;

    // simple arg parse
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--listen" && i+1 < argc) {
            listen_port = std::stoi(argv[++i]);
        } else if (a == "--peer" && i+1 < argc) {
            std::string p = argv[++i];
            auto pos = p.find(':');
            if (pos==std::string::npos) {
                std::cerr << "peer must be host:port\n"; return 1;
            }
            peer_host = p.substr(0,pos);
            peer_port = std::stoi(p.substr(pos+1));
        } else if (a == "--help" || a == "-h") {
            print_help(argv[0]); return 0;
        }
    }

    if (listen_port==0 && peer_host.empty()) {
        print_help(argv[0]); return 1;
    }

    Transport t(listen_port, peer_host, peer_port);
    t.start();

    std::atomic<bool> running{true};
    std::thread ping_thread([&](){
        while (running) {
            if (t.is_connected()) {
                Frame f;
                f.type = 3; // PING
                f.payload = "ping";
                t.send_frame(f);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    // main loop: process incoming frames and print status
    while (true) {
        Frame f;
        if (t.pop_frame(f)) {
            if (f.type == 3) {
                std::cout << "[recv] PING -> replying with PONG\n";
                Frame r; r.type = 4; r.payload = "pong";
                t.send_frame(r);
            } else if (f.type == 4) {
                std::cout << "[recv] PONG\n";
            } else if (f.type == 1) {
                std::cout << "[recv] HELLO: " << f.payload << "\n";
            } else if (f.type == 2) {
                std::cout << "[recv] ACK: " << f.payload << "\n";
            } else {
                std::cout << "[recv] unknown type=" << int(f.type) << " payload=" << f.payload << "\n";
            }
        } else {
            // no frame; sleep small
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // print simple connected state occasionally
        static int tick = 0;
        if (++tick % 20 == 0) {
            std::cout << "[status] connected=" << (t.is_connected() ? "yes":"no") << "\n";
        }
    }

    running = false;
    ping_thread.join();
    t.stop();
    return 0;
}
