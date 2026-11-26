#include "signals/signal_handler.hpp"

#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>

namespace {

int g_pipe[2] = {-1, -1};

// Async-signal-safe: write() only. No allocation, no containers, no iostream.
void on_signal(int sig) {
    if (g_pipe[1] == -1) return;
    const unsigned char b = static_cast<unsigned char>(sig);
    ssize_t rc = ::write(g_pipe[1], &b, 1);
    (void)rc;
}

// Both ends are non-blocking so neither the handler nor the drain can stall.
// CLOEXEC keeps the pipe out of every program the shell execs.
void make_nonblocking_cloexec(int fd) {
    int fl = ::fcntl(fd, F_GETFL, 0);
    if (fl >= 0) ::fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    ::fcntl(fd, F_SETFD, FD_CLOEXEC);
}

// One handler for both signals: the byte written through the pipe identifies
// which one fired. SA_RESTART resumes interrupted reads/waits where possible;
// the code still retries EINTR explicitly for the cases it cannot.
void set_handler(int sig) {
    struct sigaction sa {};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    ::sigaction(sig, &sa, nullptr);
}

}  // namespace

namespace signals {

void install() {
    if (g_pipe[0] != -1) return;
    if (::pipe(g_pipe) < 0) {
        g_pipe[0] = g_pipe[1] = -1;
        return;
    }
    make_nonblocking_cloexec(g_pipe[0]);
    make_nonblocking_cloexec(g_pipe[1]);

    // SIGCHLD: wake the main loop so it can waitpid() and update jobs.
    // SIGINT:  Ctrl+C at the prompt must not kill the shell.
    set_handler(SIGCHLD);
    set_handler(SIGINT);

    // Without full terminal job control the shell ignores these itself; the
    // foreground child (same process group, default handlers) still reacts.
    ::signal(SIGQUIT, SIG_IGN);
    ::signal(SIGTSTP, SIG_IGN);
    ::signal(SIGTTIN, SIG_IGN);
    ::signal(SIGTTOU, SIG_IGN);
}

int notify_fd() { return g_pipe[0]; }

bool drain_notifications() {
    if (g_pipe[0] == -1) return false;
    bool got_int = false;
    unsigned char buf[64];
    ssize_t n = 0;
    // Non-blocking read until EAGAIN: several signals can arrive between two
    // select() wakeups, and standard signals may coalesce into one byte.
    while ((n = ::read(g_pipe[0], buf, sizeof buf)) > 0) {
        for (ssize_t i = 0; i < n; ++i) {
            if (buf[i] == SIGINT) got_int = true;
        }
    }
    return got_int;
}

// Reset every disposition the shell changed, otherwise exec keeps SIG_IGN or
// the self-pipe handler alive in the new program. The pipe fds are closed here
// too; CLOEXEC would cover exec, but _exit() paths never reach it.
void reset_in_child() {
    struct sigaction dfl {};
    dfl.sa_handler = SIG_DFL;
    sigemptyset(&dfl.sa_mask);
    const int sigs[] = {SIGINT,  SIGQUIT, SIGTSTP, SIGTERM,
                        SIGCHLD, SIGCONT, SIGTTIN, SIGTTOU};
    for (int sig : sigs) ::sigaction(sig, &dfl, nullptr);

    for (int& fd : g_pipe) {
        if (fd != -1) {
            ::close(fd);
            fd = -1;
        }
    }
}

}  // namespace signals
