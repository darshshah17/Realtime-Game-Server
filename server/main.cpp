#include "GameServer.h"
#include <iostream>
#include <signal.h>

GameServer* g_server = nullptr;

void signalHandler(int signum) {
    if (g_server) {
        std::cout << "Shutting down server..." << std::endl;
        g_server->stop();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    
    g_server = new GameServer(port);
    
    std::cout << "Starting game server on port " << port << std::endl;
    g_server->run();
    
    delete g_server;
    return 0;
}

