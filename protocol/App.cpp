#include "App.hpp"

bool App::start() {
    // Note: order is important


    if (m_config.m_error) {
        LOG_ERR("Failed to parse config file.");
        return false;
    }

    m_ioc = new boost::asio::io_context{m_config.m_concurrency};

    // Connect to database
    m_db = std::make_unique<DB>();

    // Start apps
    m_mods.find_modules();
    if (!m_mods.start_all()) {
        LOG_ERR("Failed to start apps.");
        return false;
    }

    m_pages = std::make_unique<Pages>();

    m_server.start();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(m_config.m_concurrency - 1);
    for(auto i = m_config.m_concurrency - 1; i > 0; --i)
        v.emplace_back([this]{ m_ioc->run(); });
    m_ioc->run();

    return true;
}
