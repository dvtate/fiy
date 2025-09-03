#include <clocale>

#include <thread>

#include <gpgme.h>

#include "Server/Server.hpp"

#include "FIY.hpp"

void init_gpgme() {
    /* Initialize the locale environment.  */
    setlocale (LC_ALL, "");
    gpgme_check_version (nullptr);
    gpgme_set_locale (nullptr, LC_CTYPE, setlocale (LC_CTYPE, nullptr));
#ifdef LC_MESSAGES
    gpgme_set_locale (nullptr, LC_MESSAGES, setlocale (LC_MESSAGES, nullptr));
#endif
}

bool FIY::start() {
    // Note: order is important

    // Initialize gpgme library
    init_gpgme();

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
