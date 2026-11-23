#include "executor/executor.hpp"

#include "ipc/ipc.hpp"
#include "monitor/process_monitor.hpp"
#include "process/terminal.hpp"
#include "signals/signal_handler.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cctype>
#include <iostream>

namespace {

int parse_signal(const std::string& s) {
    if (s.empty()) return -1;
    if (std::isdigit(static_cast<unsigned char>(s[0]))) return std::atoi(s.c_str());
    if (s == "HUP") return SIGHUP;
    if (s == "INT") return SIGINT;
    if (s == "KILL") return SIGKILL;
    if (s == "TERM") return SIGTERM;
    if (s == "STOP") return SIGSTOP;
    if (s == "CONT") return SIGCONT;
    return -1;
}

}  // namespace

bool Executor::run(const ParsedLine& line, const std::string& raw) {
    if (!line.ok) {
        std::cerr << "minishell: " << line.error << "\n";
        return true;
    }
    if (line.stages.empty()) return true;

    if (line.stages.size() == 1) {
        bool handled = false;
        const bool keep_running = run_builtin(line, handled);
        if (handled) return keep_running;
    }
    run_external(line, raw);
    return true;
}

bool Executor::run_builtin(const ParsedLine& line, bool& handled) {
    handled = true;
    const Command& c = line.stages[0];
    const std::string& name = c.program;

    if (name == "cd") {
        const char* home = std::getenv("HOME");
        const char* target = c.args.empty() ? (home ? home : "/") : c.args[0].c_str();
        if (::chdir(target) < 0) std::perror("minishell: cd");
    } else if (name == "pwd") {
        char buf[4096];
        if (::getcwd(buf, sizeof buf)) {
            std::cout << buf << "\n";
        } else {
            std::perror("minishell: pwd");
        }
    } else if (name == "exit") {
        return false;
    } else if (name == "jobs") {
        jobs_.print();
    } else if (name == "history") {
        history_.print();
    } else if (name == "kill") {
        if (c.args.empty()) {
            std::cerr << "usage: kill <pid|%job> [signal]\n";
        } else {
            const int sig = c.args.size() >= 2 ? parse_signal(c.args[1]) : SIGTERM;
            if (sig <= 0) {
                std::cerr << "minishell: kill: bad signal\n";
            } else {
                pid_t pid = -1;
                const std::string& target = c.args[0];
                if (target[0] == '%') {
                    if (Job* j = jobs_.find(std::atoi(target.c_str() + 1))) {
                        if (!j->pids.empty()) pid = j->pids.front();
                    }
                } else {
                    pid = static_cast<pid_t>(std::atoi(target.c_str()));
                }
                if (pid <= 0) {
                    std::cerr << "minishell: kill: invalid target\n";
                } else if (::kill(pid, sig) < 0) {
                    std::perror("minishell: kill");
                } else {
                    std::cout << "sent signal " << sig << " to " << pid << "\n";
                }
            }
        }
    } else if (name == "fg" || name == "bg") {
        Job* j = nullptr;
        if (!c.args.empty()) {
            std::string id = c.args[0];
            if (!id.empty() && id[0] == '%') id = id.substr(1);
            j = jobs_.find(std::atoi(id.c_str()));
        }
        if (!j) {
            std::cerr << "minishell: " << name << ": no such job\n";
        } else {
            const std::vector<pid_t> pids = j->pids;
            const int id = j->id;
            const std::string command = j->command;
            const pid_t pgid = pids.empty() ? 0 : pids.front();
            j->state = JobState::Running;
            std::cout << command << "\n";

            if (name == "fg") {
                jobs_.erase(id);
                if (terminal::active() && pgid > 0) terminal::give_to(pgid);
                for (pid_t pid : pids) ::kill(pid, SIGCONT);
                wait_foreground(pids, command);
                if (terminal::active()) terminal::take_back();
            } else {
                // bg: stay in the table, just let it run again.
                for (pid_t pid : pids) ::kill(pid, SIGCONT);
            }
        }
    } else if (name == "ps") {
        procmon::print_ps(::getpid());
    } else if (name == "pinfo") {
        if (c.args.empty()) {
            std::cerr << "usage: pinfo <pid>\n";
        } else {
            procmon::print_pinfo(static_cast<pid_t>(std::atoi(c.args[0].c_str())));
        }
    } else if (name == "monitor") {
        if (c.args.size() >= 2 && c.args[0] == "--pid") {
            procmon::print_monitor_pid(static_cast<pid_t>(std::atoi(c.args[1].c_str())));
        } else {
            procmon::print_monitor();
        }
    } else if (name == "help") {
        std::cout
            << "minishell builtins:\n"
            << "  cd [dir]            change directory (default: $HOME)\n"
            << "  pwd                 print working directory\n"
            << "  exit                leave the shell\n"
            << "  jobs                list background/stopped jobs\n"
            << "  history             show command history\n"
            << "  kill <pid|%job> [sig]  send a signal (default SIGTERM)\n"
            << "  fg <id> / bg <id>   resume a job (SIGCONT), fg also waits\n"
            << "  ps                  processes created by this shell\n"
            << "  pinfo <pid>         detailed process information\n"
            << "  monitor [--pid N]   system process table\n"
            << "  help                this text\n";
    } else {
        handled = false;
    }
    return true;
}

void Executor::run_external(const ParsedLine& line, const std::string& raw) {
    const size_t count = line.stages.size();
    const bool jc = terminal::active();
    const bool own_group = line.background || jc;
    // The job table shows the command without the trailing '&'.
    std::string display = raw;
    while (!display.empty() && (display.back() == '&' || display.back() == ' '))
        display.pop_back();
    std::vector<pid_t> pids;
    pids.reserve(count);
    pid_t job_pgid = 0;
    int prev_read = -1;

    for (size_t i = 0; i < count; ++i) {
        int pipefd[2] = {-1, -1};
        const bool has_next = i + 1 < count;
        if (has_next && ::pipe(pipefd) < 0) {
            std::perror("minishell: pipe");
            break;
        }

        const pid_t pid = ::fork();
        if (pid == 0) {
            signals::reset_in_child();
            if (own_group) ::setpgid(0, 0);

            if (prev_read != -1) {
                ::dup2(prev_read, STDIN_FILENO);
                ::close(prev_read);
            }
            if (has_next) {
                ::close(pipefd[0]);
                ::dup2(pipefd[1], STDOUT_FILENO);
                ::close(pipefd[1]);
            }
            if (!ipc::apply_redirection(line.stages[i])) ::_exit(1);

            const Command& c = line.stages[i];
            std::vector<char*> argv;
            argv.push_back(const_cast<char*>(c.program.c_str()));
            for (const std::string& a : c.args) argv.push_back(const_cast<char*>(a.c_str()));
            argv.push_back(nullptr);

            ::execvp(c.program.c_str(), argv.data());
            std::fprintf(stderr, "minishell: %s: %s\n", c.program.c_str(),
                         std::strerror(errno));
            ::_exit(127);
        }
        if (pid < 0) {
            std::perror("minishell: fork");
            if (has_next) ::close(pipefd[1]);
            break;
        }
        if (own_group) ::setpgid(pid, pid);
        if (jc && !line.background && job_pgid == 0) job_pgid = pid;

        pids.push_back(pid);
        if (prev_read != -1) ::close(prev_read);
        if (has_next) ::close(pipefd[1]);
        prev_read = has_next ? pipefd[0] : -1;
    }
    if (prev_read != -1) ::close(prev_read);

    if (line.background) {
        if (!pids.empty()) {
            const int id = jobs_.add(pids, display);
            std::cout << "[" << id << "] Started  " << display << "  PID: " << pids.front()
                      << "\n";
        }
    } else if (!pids.empty()) {
        if (job_pgid > 0) terminal::give_to(job_pgid);
        wait_foreground(pids, raw);
        if (job_pgid > 0) terminal::take_back();
    }
}

void Executor::wait_foreground(const std::vector<pid_t>& pids, const std::string& raw) {
    for (pid_t pid : pids) {
        int status = 0;
        pid_t rc;
        do {
            rc = ::waitpid(pid, &status, WUNTRACED);
        } while (rc < 0 && errno == EINTR);

        if (rc < 0) continue;
        if (WIFSTOPPED(status)) {
            const int id = jobs_.add({pid}, raw);
            if (Job* j = jobs_.find(id)) j->state = JobState::Stopped;
            std::cout << "\n[" << id << "] Stopped  " << raw << "  PID: " << pid << "\n";
            return;  // remaining pipeline stages are left stopped/running
        }
    }
}
