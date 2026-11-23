#include "ipc/ipc.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace ipc {

namespace {

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
