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
#include <climits>
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

// strtol-based: atoi() is undefined behavior on overflow.
bool parse_long(const std::string& s, long& out) {
    if (s.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(s.c_str(), &end, 10);
    if (errno != 0 || end == s.c_str() || *end != '\0') return false;
    out = value;
    return true;
}

bool parse_pid(const std::string& s, pid_t& out) {
    long value = 0;
    if (!parse_long(s, value) || value <= 0 || value > INT_MAX) return false;
    out = static_cast<pid_t>(value);
    return true;
}

bool parse_job_id(const std::string& s, int& out) {
    long value = 0;
    if (!parse_long(s, value) || value <= 0 || value > INT_MAX) return false;
    out = static_cast<int>(value);
    return true;
}

// Signal a whole job: one killpg for the group, per-pid as a fallback for jobs
// that were started without job control (their children share the shell's
// group, so killpg(pgid) would be wrong).
void signal_job(const std::vector<pid_t>& pids, int sig) {
    if (pids.empty()) return;
    if (::killpg(pids.front(), sig) == 0) return;
    for (pid_t pid : pids) ::kill(pid, sig);
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
            std::cerr << "usage: kill [-sig] <pid|%job> [sig]\n";
            return true;
        }
        std::string target;
        int sig = SIGTERM;
        if (c.args.size() >= 2 && !c.args[0].empty() && c.args[0][0] == '-' &&
            c.args[0].size() > 1) {
            // bash-style prefix: kill -9 pid / kill -KILL pid
            sig = parse_signal(c.args[0].substr(1));
            target = c.args[1];
        } else {
            target = c.args[0];
            if (c.args.size() >= 2) sig = parse_signal(c.args[1]);
        }
        if (sig <= 0) {
            std::cerr << "minishell: kill: bad signal\n";
            return true;
        }
        if (!target.empty() && target[0] == '%') {
            int job_id = 0;
            Job* j = parse_job_id(target.substr(1), job_id) ? jobs_.find(job_id) : nullptr;
            if (!j || j->pids.empty()) {
                std::cerr << "minishell: kill: no such job\n";
            } else {
                signal_job(j->pids, sig);
                std::cout << "sent signal " << sig << " to job " << job_id << "\n";
            }
        } else {
            pid_t pid = 0;
            if (!parse_pid(target, pid)) {
                std::cerr << "minishell: kill: invalid pid\n";
            } else if (::kill(pid, sig) < 0) {
                std::perror("minishell: kill");
            } else {
                std::cout << "sent signal " << sig << " to " << pid << "\n";
            }
        }
    } else if (name == "fg" || name == "bg") {
        Job* j = nullptr;
        if (!c.args.empty()) {
            std::string id = c.args[0];
            if (!id.empty() && id[0] == '%') id = id.substr(1);
            int job_id = 0;
            if (parse_job_id(id, job_id)) j = jobs_.find(job_id);
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
                signal_job(pids, SIGCONT);
                wait_foreground(pids, command);
                if (terminal::active()) terminal::take_back();
            } else {
                // bg: stay in the table, just let it run again.
                signal_job(pids, SIGCONT);
            }
        }
    } else if (name == "ps") {
        procmon::print_ps(::getpid());
    } else if (name == "pinfo") {
        pid_t pid = 0;
        if (c.args.empty()) {
            std::cerr << "usage: pinfo <pid>\n";
        } else if (!parse_pid(c.args[0], pid)) {
            std::cerr << "minishell: pinfo: invalid pid\n";
        } else {
            procmon::print_pinfo(pid);
        }
    } else if (name == "monitor") {
        if (c.args.size() >= 2 && c.args[0] == "--pid") {
            pid_t pid = 0;
            if (!parse_pid(c.args[1], pid)) {
                std::cerr << "minishell: monitor: invalid pid\n";
            } else {
                procmon::print_monitor_pid(pid);
            }
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
            << "  kill [-sig] <pid|%job> [sig]  send a signal (default SIGTERM)\n"
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
            // Every stage joins stage 0's group so the job can be signalled,
            // resumed and handed the terminal as one unit.
            if (own_group) ::setpgid(0, i == 0 ? 0 : pids.front());

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
            if (has_next) {
                ::close(pipefd[0]);
                ::close(pipefd[1]);
            }
            break;
        }
        if (own_group) ::setpgid(pid, pids.empty() ? pid : pids.front());
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
            // All stages of the pipeline share the group, so record every pid;
            // a later `fg %n` then waits for the whole job.
            const int id = jobs_.add(pids, raw);
            if (Job* j = jobs_.find(id)) j->state = JobState::Stopped;
            std::cout << "\n[" << id << "] Stopped  " << raw << "  PID: " << pid << "\n";
            return;
        }
    }
}
