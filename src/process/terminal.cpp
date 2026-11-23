#include "process/terminal.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>

namespace terminal {

namespace {

int g_tty = -1;
pid_t g_shell_pgid = 0;
bool g_active = false;

void tcsetpgrp_retry(int fd, pid_t pgid) {
    pid_t rc;
    do {
        rc = ::tcsetpgrp(fd, pgid);
    } while (rc < 0 && errno == EINTR);
}

}  // namespace

bool init() {
    if (!::isatty(STDIN_FILENO)) return false;

    g_tty = ::open("/dev/tty", O_RDWR);
    if (g_tty < 0) return false;

    g_shell_pgid = ::getpid();
    // Fails with EPERM when the shell is already a session leader; its pgid is
    // the pid in that case, so nothing is lost.
    if (::setpgid(0, g_shell_pgid) < 0 && errno != EPERM) {
        ::close(g_tty);
        g_tty = -1;
        return false;
    }

    tcsetpgrp_retry(g_tty, g_shell_pgid);
    if (::tcgetpgrp(g_tty) != g_shell_pgid) {
        ::close(g_tty);
        g_tty = -1;
        return false;
    }

    g_active = true;
    return true;
}

bool active() { return g_active; }

pid_t shell_pgid() { return g_shell_pgid; }

void give_to(pid_t pgid) {
    if (!g_active) return;
    tcsetpgrp_retry(g_tty, pgid);
}

void take_back() {
    if (!g_active) return;
    tcsetpgrp_retry(g_tty, g_shell_pgid);
}

}  // namespace terminal
