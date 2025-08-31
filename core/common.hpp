#pragma once
#include <string>
#include <iostream>

struct Options{
    std::string role; // "Writer" or "Reader" (Currently only one role is assigned to one person)
    std::string host; // IP addr of peer host
    int port = 5000;  // Default tcp port of peer 
    bool help = false;// When user needs help
};

inline Options parse_args(int argc,char** argv){
    Options opt;
    if(argc<=1){ // only program name is there ; user knows no shit help them
        opt.help = true;
        return opt;
    }

    for(int i=1;i<argc;i++){
        std::string arg = argv[i];
        if(arg == "--help" || arg == "-h"){
            opt.help = true;
        }
        else if(arg == "--role" && (i+1)<argc){
            opt.role = argv[++i];
        }
        else if(arg == "--host" && (i+1)<argc){
            opt.host = argv[++i];
        }
        else if(arg == "--port" && (i+1)<argc){
            opt.port = std::stoi(argv[++i]);
        }
    }
    return opt;
}

inline void print_help(const char* prog) {
    std::cout << "Usage: " << prog 
              << " --role [writer|reader] --host <peer_host> --port <port>\n";
}