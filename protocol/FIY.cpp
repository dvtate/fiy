#include <clocale>

#include <thread>

#include "Server/Server.hpp"

#include "FIY.hpp"

bool FIY::start() {
    // Note: order is important

    if (m_config.m_error) {
        LOG_ERR("Failed to parse config file.");
        return false;
    }

    // Track current time
    std::thread now_tracker{[this](){
        while (true) {
            m_now = std::time(nullptr);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }};

    // Start apps
    m_mods.find_modules();
    if (!m_mods.start_all()) {
        LOG_ERR("Failed to start apps.");
        return false;
    }

    m_pages = std::make_unique<Pages>();

    // Make Boost IO context
    m_ioc = new boost::asio::io_context{m_config.m_concurrency};

    // Initialize the server (it uses boost io context)
    Server::start();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(m_config.m_concurrency - 1);
    for(auto i = m_config.m_concurrency - 1; i > 0; --i)
        v.emplace_back([this]{ m_ioc->run(); });
    m_ioc->run();

    return true;
}
