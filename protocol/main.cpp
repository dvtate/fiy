
#include "drogon/drogon.h"

#include "App.hpp"

// Global App singleton
App* g_app;

int main(int argc, char** argv) {
    // Start app services
    g_app = (argc >= 2) ? new App(argv[1]) : new App(); // specify path to config file
    g_app->start();
//
//    using namespace std::chrono_literals;
//    drogon::app()
//        .enableSession(48h)
////        .setLogLevel(trantor::Logger::kTrace)
//        .setLogLevel(trantor::Logger::kDebug)
//        .addListener("127.0.0.1", g_app->m_config.m_port)
////        .enableRunAsDaemon()
//        .run();
}