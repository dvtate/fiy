//
// Created by tate on 7/16/24.
//

#include <wait.h>
#include <csignal>
#include <iostream>

#include "BackgroundProcess.hpp"

void BackgroundProcess::start() {
    // Fork
    m_pid = fork();
    if (m_pid == -1) {
        perror("fork() failed");
        return;
    }

    // Parent
    if (m_pid == 0) {
        return;
    }

    // Child process
    execlp(m_path.data(), m_path.data());
    // should be unreachable:
    perror("execlp() failed");
    std::cerr << "Failed to exec: " << m_path << std::endl;
    exit(EXIT_FAILURE);
}

int BackgroundProcess::wait() {
    int ret;
    if (waitpid(m_pid, &ret, 0) == -1)
        perror("waitpid() failed");
    m_pid = -1;
    return ret;
}

void BackgroundProcess::stop() {
    if (kill(m_pid, SIGTERM) == -1)
        perror("kill() failed");
}
