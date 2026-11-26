#include "ipc/ipc.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace ipc {

namespace {

// dup2() atomically replaces `target` with a copy of `fd`, closing whatever
// `target` used to be. The original fd is no longer needed after the copy, and
// leaving it open would keep a pipe/file alive across exec.
bool move_fd(int fd, int target, const std::string& path) {
    if (::dup2(fd, target) < 0) {
        std::fprintf(stderr, "minishell: %s: %s\n", path.c_str(), std::strerror(errno));
        ::close(fd);
        return false;
    }
    ::close(fd);
    return true;
}

}  // namespace

// Runs in the child after fork(), before execvp(): changing fd 0/1 here only
// affects the new program, never the shell. Returns false after printing the
// reason so the caller can _exit without exec'ing the wrong fds.
bool apply_redirection(const Command& cmd) {
    if (!cmd.infile.empty()) {
        int fd = ::open(cmd.infile.c_str(), O_RDONLY);
        if (fd < 0) {
            std::fprintf(stderr, "minishell: %s: %s\n", cmd.infile.c_str(),
                         std::strerror(errno));
            return false;
        }
        if (!move_fd(fd, STDIN_FILENO, cmd.infile)) return false;
    }

    if (!cmd.outfile.empty()) {
        // 0644 is further restricted by the process umask. Only stdout and
        // stdin are redirected; the shell has no `2>` syntax by design.
        int flags = O_WRONLY | O_CREAT | (cmd.append ? O_APPEND : O_TRUNC);
        int fd = ::open(cmd.outfile.c_str(), flags, 0644);
        if (fd < 0) {
            std::fprintf(stderr, "minishell: %s: %s\n", cmd.outfile.c_str(),
                         std::strerror(errno));
            return false;
        }
        if (!move_fd(fd, STDOUT_FILENO, cmd.outfile)) return false;
    }
    return true;
}

}  // namespace ipc
