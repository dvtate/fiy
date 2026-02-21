#include "FIY.hpp"

// Global App singleton
FIY* g_fiy;

int main(const int argc, char** argv) {
    // Start services
    g_fiy = (argc >= 2) ? new FIY(argv[1]) : new FIY(); // specify path to config file
    g_fiy->start();
}