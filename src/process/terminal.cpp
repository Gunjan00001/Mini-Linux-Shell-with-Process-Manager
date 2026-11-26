#include "process/terminal.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>

namespace terminal {

namespace {

int g_tty = -1;
pid_t g_shell_pgid = 0;
bool g_active = false;

// tcsetpgrp() can be interrupted by a signal; retry only that case. Other
// errors (typically EPERM in a session without a controlling terminal) leave
// the old owner in place, which is safe: the caller never assumes success.
void tcsetpgrp_retry(int fd, pid_t pgid) {
    pid_t rc;
    do {
        rc = ::tcsetpgrp(fd, pgid);
    } while (rc < 0 && errno == EINTR);
}

}  // namespace

bool init() {
    // Pipes, CI runners and IDE consoles are not terminals. Without a tty the
    // kernel has no foreground group to manage, so job control is disabled.
    if (!::isatty(STDIN_FILENO)) return false;

    // Use the controlling terminal directly instead of fd 0/1: those may be
    // redirected by the user, while /dev/tty always names the session's tty.
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

    // tcsetpgrp does not prove ownership; read it back. If the handoff failed
    // there is no safe way to give the terminal to a job later.
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

// Foreground job: the terminal's signals (Ctrl+C, Ctrl+Z) now belong to the
// job's process group. Called before the job is continued or waited on.
void give_to(pid_t pgid) {
    if (!g_active) return;
    tcsetpgrp_retry(g_tty, pgid);
}

// The job exited or stopped; reclaim the terminal before printing a prompt so
// the shell, not the dead group, receives the next Ctrl+C.
void take_back() {
    if (!g_active) return;
    tcsetpgrp_retry(g_tty, g_shell_pgid);
}

}  // namespace terminal
