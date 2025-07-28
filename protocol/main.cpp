

#include "App.hpp"

// Global App singleton
App* g_app;

int main(int argc, char** argv) {
    // Start app services
    g_app = (argc >= 2) ? new App(argv[1]) : new App(); // specify path to config file
    g_app->start();
}