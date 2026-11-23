#pragma once

#include <string>

#include "executor/executor.hpp"

class Shell {
public:
    void run();

private:
    std::string prompt() const;
    // Reads one line from the terminal, serviced by select() so SIGCHLD
    // notifications are handled while the user is idle at the prompt.
    bool read_line(std::string& out);

    Executor exec_;
};
