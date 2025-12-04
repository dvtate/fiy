//
// Created by tate on 12/2/25.
//

#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

/**
 * CGI Script Runner
 *
 * @remark this solution is designed to be easy to use, not micro-optimized
 */
struct CGI {
    // -- Environment variables --
    std::string AUTH_TYPE;        // 4.1.1.  AUTH_TYPE
    std::string CONTENT_LENGTH;   // 4.1.2.  CONTENT_LENGTH
    std::string CONTENT_TYPE;     // 4.1.3.  CONTENT_TYPE
    static constexpr const char*
        GATEWAY_INTERFACE{"CGI/1.1"}; // 4.1.4.  GATEWAY_INTERFACE
    std::string PATH_INFO;        // 4.1.5.  PATH_INFO
    std::string PATH_TRANSLATED;  // 4.1.6.  PATH_TRANSLATED
    std::string QUERY_STRING;     // 4.1.7.  QUERY_STRING
    std::string REMOTE_ADDR;      // 4.1.8.  REMOTE_ADDR
    std::string REMOTE_HOST;      // 4.1.9.  REMOTE_HOST
    std::string REMOTE_IDENT;     // 4.1.10. REMOTE_IDENT
    std::string REMOTE_USER;      // 4.1.11. REMOTE_USER
    std::string REQUEST_METHOD;   // 4.1.12. REQUEST_METHOD
    std::string SCRIPT_NAME;      // 4.1.13. SCRIPT_NAME
    std::string SERVER_NAME;      // 4.1.14. SERVER_NAME
    std::string SERVER_PORT;      // 4.1.15. SERVER_PORT
    std::string SERVER_PROTOCOL{"HTTP/1.1"};  // 4.1.16. SERVER_PROTOCOL
    std::string SERVER_SOFTWARE;  // 4.1.17. SERVER_SOFTWARE

    std::vector<std::string> m_env;

    std::vector<std::string> args;
    std::string body;

    explicit CGI(
        const std::vector<std::string>& argv
    ):
        SCRIPT_NAME(argv[0]),
        args(argv)
    {}

    struct Result {
        int status{0};
        std::string stdout{};
    };

    void set_env(const std::string& var, const std::string& val) {
        m_env.emplace_back(var + "=" + val);
    }

    Result run() const {
        // Construct environment variables
        std::vector<std::string> env = m_env;
        static const std::string gateway_interface =
            std::string("GATEWAY_INTERFACE=") + GATEWAY_INTERFACE;
        env.push_back(gateway_interface);
        if (!AUTH_TYPE.empty())
            env.emplace_back("AUTH_TYPE=" + AUTH_TYPE);
        if (!CONTENT_LENGTH.empty())
            env.emplace_back("CONTENT_LENGTH=" + CONTENT_LENGTH);
        if (!CONTENT_TYPE.empty())
            env.emplace_back("CONTENT_TYPE=" + CONTENT_TYPE);
        if (!PATH_INFO.empty())
            env.emplace_back("PATH_INFO=" + PATH_INFO);
        if (!PATH_TRANSLATED.empty())
            env.emplace_back("PATH_TRANSLATED=" + PATH_TRANSLATED);
        if (!QUERY_STRING.empty())
            env.emplace_back("QUERY_STRING=" + QUERY_STRING);
        if (!REMOTE_ADDR.empty())
            env.emplace_back("REMOTE_ADDR=" + REMOTE_ADDR);
        if (!REMOTE_HOST.empty())
            env.emplace_back("REMOTE_HOST=" + REMOTE_HOST);
        if (!REMOTE_IDENT.empty())
            env.emplace_back("REMOTE_IDENT=" + REMOTE_IDENT);
        if (!REMOTE_USER.empty())
            env.emplace_back("REMOTE_USER=" + REMOTE_USER);
        if (!REQUEST_METHOD.empty())
            env.emplace_back("REQUEST_METHOD=" + REQUEST_METHOD);
        if (!SCRIPT_NAME.empty())
            env.emplace_back("SCRIPT_NAME=" + SCRIPT_NAME);
        if (!SERVER_NAME.empty())
            env.emplace_back("SERVER_NAME=" + SERVER_NAME);
        if (!SERVER_PORT.empty())
            env.emplace_back("SERVER_PORT=" + SERVER_PORT);
        if (!SERVER_PROTOCOL.empty())
            env.emplace_back("SERVER_PROTOCOL=" + SERVER_PROTOCOL);
        if (!SERVER_SOFTWARE.empty())
            env.emplace_back("SERVER_SOFTWARE=" + SERVER_SOFTWARE);

        // std::cout <<"env.size() = " << env.size() << std::endl;
        // for (auto& e : env)
        //     std::cout <<"env: " <<e <<std::endl;
        return run_cmd(args, env, body);
    }

protected:
    static Result run_cmd(
        const std::vector<std::string>& argv,
        const std::vector<std::string>& env,
        const std::string& stdin_input
    ) {
        // TODO timeout

        // --- Create pipes ---
        int pipe_stdin[2], pipe_stdout[2];
        if (pipe(pipe_stdin) < 0 || pipe(pipe_stdout) < 0)
            throw std::runtime_error("pipe() failed");

        const pid_t pid = fork();
        if (pid < 0)
            throw std::runtime_error("fork() failed");

        if (pid == 0) {
            // --- Child process ---
            // Redirect stdin/out
            dup2(pipe_stdin[0], STDIN_FILENO);
            dup2(pipe_stdout[1], STDOUT_FILENO);

            // Close unused ends
            close(pipe_stdin[1]);
            close(pipe_stdout[0]);

            // Build argv array
            std::vector<char*> c_argv;
            c_argv.reserve(argv.size() + 1);
            for (auto& s : argv)
                c_argv.push_back(const_cast<char*>(s.c_str()));
            c_argv.push_back(nullptr);

            // Build environment array
            std::vector<char*> c_env;
            c_env.reserve(env.size() + 1);
            for (auto& e : env)
                c_env.push_back(const_cast<char*>(e.c_str()));
            c_env.push_back(nullptr);

            // exec
            execvpe(c_argv[0], c_argv.data(), c_env.data());
            _exit(127); // execvpe failed
        }

        // --- Parent process ---
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);

        // Write stdin
        if (!stdin_input.empty())
            if (write(pipe_stdin[1], stdin_input.data(), stdin_input.size()) < 0)
                perror("write()");
        close(pipe_stdin[1]); // send EOF

        // Read stdout
        Result ret;
        char buffer[4096];
        ssize_t n;
        while (0 < (n = read(pipe_stdout[0], buffer, sizeof(buffer))))
            ret.stdout.append(buffer, n);
        close(pipe_stdout[0]);

        // Wait for child
        waitpid(pid, &ret.status, 0);
        ret.status = WIFEXITED(ret.status)
            ? WEXITSTATUS(ret.status)
            : -1;

        return ret;
    }
};
