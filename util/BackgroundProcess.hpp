//
// Created by tate on 7/16/24.
//

#pragma once

#include <unistd.h>
#include <string>

/**
 * Child process management
 */
class BackgroundProcess {
public:
    pid_t m_pid;
    const std::string m_path;

    explicit BackgroundProcess(std::string path) : m_path(std::move(path)) {}

    void start();

    int wait();

    void stop();
};
