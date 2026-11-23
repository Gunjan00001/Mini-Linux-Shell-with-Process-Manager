#pragma once

#include <sys/types.h>

#include <string>
#include <vector>

enum class JobState { Running, Stopped, Done };

// A job = one background command or one stopped foreground job. A pipeline
// keeps every stage pid so fg/bg/kill can resume or signal the whole thing.
struct Job {
    int id = 0;
    std::vector<pid_t> pids;
    std::string command;
    JobState state = JobState::Running;
};

class JobManager {
public:
    int add(std::vector<pid_t> pids, const std::string& command);

    // Drains finished/stopped children with waitpid(-1, WNOHANG). Only called
    // from the main loop, never from a signal handler. Fills `notices` with
    // user-facing "[1] Done ..." lines.
    void reap(std::vector<std::string>& notices);

    void print() const;
    void erase(int id);
    void terminate_all();  // on shell exit: SIGHUP every tracked process

    Job* find(int id);
    bool empty() const { return jobs_.empty(); }
    const std::vector<Job>& jobs() const { return jobs_; }

private:
    Job* find_by_pid(pid_t pid);
    std::vector<Job> jobs_;
    int next_id_ = 1;
};
