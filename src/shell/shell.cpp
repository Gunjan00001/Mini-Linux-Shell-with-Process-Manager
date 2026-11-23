#include "shell/shell.hpp"

#include "parser/parser.hpp"
#include "signals/signal_handler.hpp"

#include <sys/select.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>

void Shell::run() {
    while (true) {
        std::vector<std::string> notices;
        exec_.jobs().reap(notices);
        for (const std::string& n : notices) std::cout << n << "\n";

        std::cout << prompt() << std::flush;

        std::string line;
        if (!read_line(line)) break;
        if (line.empty()) continue;

        // `!N` recalls entry N from history before it is executed.
        if (line.size() > 1 && line[0] == '!' &&
            std::all_of(line.begin() + 1, line.end(), [](unsigned char c) {
                return std::isdigit(c) != 0;
            })) {
            const std::string* previous =
                exec_.history().at(static_cast<size_t>(std::stoul(line.substr(1))));
            if (!previous) {
                std::cerr << "minishell: " << line << ": event not found\n";
                continue;
            }
            line = *previous;
            std::cout << line << "\n" << std::flush;
        }

        exec_.history().add(line);
        const ParsedLine parsed = parse_line(line);
        if (!exec_.run(parsed, line)) break;
    }

    // Normal shutdown: don't leave our background jobs behind.
    exec_.jobs().terminate_all();
    std::cout << "\n";
}

std::string Shell::prompt() const {
    const char* user = std::getenv("USER");
    if (!user) user = "user";

    char buf[4096];
    std::string dir = ::getcwd(buf, sizeof buf) ? buf : "?";
    const char* home = std::getenv("HOME");
    if (home) {
        const size_t len = std::strlen(home);
        if (dir.compare(0, len, home) == 0) dir = "~" + dir.substr(len);
    }
    return std::string(user) + "@minishell:" + dir + "$ ";
}

bool Shell::read_line(std::string& out) {
    out.clear();
    const int sig_fd = signals::notify_fd();

    while (true) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        int maxfd = STDIN_FILENO;
        if (sig_fd >= 0) {
            FD_SET(sig_fd, &rfds);
            maxfd = std::max(maxfd, sig_fd);
        }

        const int ready = ::select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
        if (ready < 0) {
            if (errno == EINTR) continue;
            return false;
        }

        if (sig_fd >= 0 && FD_ISSET(sig_fd, &rfds)) {
            signals::drain_notifications();
            std::vector<std::string> notices;
            exec_.jobs().reap(notices);

            // Only speak up when a job actually changed state. Re-printing the
            // prompt here duplicated it after every external command.
            if (!notices.empty()) {
                std::cout << "\n";
                for (const std::string& n : notices) std::cout << n << "\n";
            }
            continue;
        }

        if (!FD_ISSET(STDIN_FILENO, &rfds)) continue;

        char c = 0;
        const ssize_t n = ::read(STDIN_FILENO, &c, 1);
        if (n == 0) return false;  // Ctrl+D
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (c == '\n') return true;
        out += c;
    }
}
