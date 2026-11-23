#include "shell/shell.hpp"
#include "process/terminal.hpp"
#include "signals/signal_handler.hpp"

int main() {
    // Ignore SIGTTOU before touching the terminal; then take it over.
    signals::install();
    terminal::init();

    Shell shell;
    shell.run();
    return 0;
}
