#include "shell/shell.hpp"
#include "process/terminal.hpp"
#include "signals/signal_handler.hpp"

int main() {
    // Ignore SIGTTOU before touching the terminal; then take it over.
    signals::install();
    // Failure is expected when stdin is not a tty; job control then no-ops.
    terminal::init();

    Shell shell;
    shell.run();
    return 0;
}
