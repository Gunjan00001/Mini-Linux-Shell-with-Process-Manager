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
        // Reap before drawing the prompt: a job that finished while the last
        // command ran should print its Done notice above the new prompt, not
        // in the middle of the user's next line.
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
            // Bound the digits before stoul(): a 20-digit "history index" used
            // to throw std::out_of_range and abort the shell.
            const std::string digits = line.substr(1);
            const std::string* previous = nullptr;
            if (digits.size() <= 9) {
                previous = exec_.history().at(static_cast<size_t>(std::stoul(digits)));
            }
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
    // USER/HOME are inherited from the parent environment; missing values get
    // harmless fallbacks so the prompt cannot throw or print "(null)".
    const char* user = std::getenv("USER");
    if (!user) user = "user";

    char buf[4096];
    std::string dir = ::getcwd(buf, sizeof buf) ? buf : "?";
    const char* home = std::getenv("HOME");
    if (home) {
        // Display-only shortcut: `/home/me/work` becomes `~/work`. The real cwd
        // is never changed; `pwd` still prints the full path.
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

        // Wait on stdin and the self-pipe together. A plain blocking read()
        // would not notice a background job finishing until the user pressed
        // Enter, which is exactly how shells end up printing Done at the wrong
        // moment.
        const int ready = ::select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
        if (ready < 0) {
            if (errno == EINTR) continue;
            return false;
        }

        if (sig_fd >= 0 && FD_ISSET(sig_fd, &rfds)) {
            const bool interrupted = signals::drain_notifications();
            std::vector<std::string> notices;
            exec_.jobs().reap(notices);

            // Only speak up when a job actually changed state. Re-printing the
            // prompt here duplicated it after every external command.
            if (!notices.empty()) {
                std::cout << "\n";
                for (const std::string& n : notices) std::cout << n << "\n";
            }
            // Ctrl+C at the prompt: drop any partial line and show a fresh
            // prompt, like bash, instead of leaving the cursor on a blank line.
            if (interrupted) {
                out.clear();
                std::cout << "\n" << prompt() << std::flush;
            }
            continue;
        }

        if (!FD_ISSET(STDIN_FILENO, &rfds)) continue;

        // Read one byte at a time: the loop must return to select() often enough
        // to service the signal pipe. No line editing or echo control here; the
        // terminal's canonical mode does the echoing.
        char c = 0;
        const ssize_t n = ::read(STDIN_FILENO, &c, 1);
        if (n == 0) return false;  // Ctrl+D: EOF -> leave the shell
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (c == '\n') return true;
        out += c;
    }
}
