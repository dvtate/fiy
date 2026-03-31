#include "FIY.hpp"

// Global App singleton
FIY* g_fiy;

int main(const int argc, char** argv) {
    // Start services
    g_fiy = new FIY();
    if (!g_fiy->start(argc, argv)) {
        LOG_ERR("Failed to start FIY");
    }
    delete g_fiy;
}