#include "process/job_manager.hpp"

#include <csignal>
#include <sys/wait.h>

#include <algorithm>
#include <iostream>

namespace {

const char* state_name(JobState s) {
    switch (s) {
        case JobState::Running: return "Running";
        case JobState::Stopped: return "Stopped";
        case JobState::Done: return "Done";
    }
    return "?";
}

}  // namespace

// Ids are never reused within a session, so `fg 1` cannot silently target a
// different job after the first one finishes.
int JobManager::add(std::vector<pid_t> pids, const std::string& command) {
    Job j;
    j.id = next_id_++;
    j.pids = std::move(pids);
    j.command = command;
    j.state = JobState::Running;
    jobs_.push_back(std::move(j));
    return jobs_.back().id;
}

// Drain every child that changed state without blocking. WUNTRACED reports
// Ctrl+Z, WCONTINUED reports `bg`/`fg`; without them the job table would only
// learn about exits. Safe only from normal code, never from the SIGCHLD handler.
void JobManager::reap(std::vector<std::string>& notices) {
    while (true) {
        int status = 0;
        pid_t pid = ::waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (pid <= 0) break;

        Job* job = find_by_pid(pid);
        if (!job) continue;  // foreground child already collected by the executor

        if (WIFSTOPPED(status)) {
            // Only announce the first stop: every stage of a pipeline stops
            // together, but the user needs one line, not one per stage.
            if (job->state != JobState::Stopped) {
                job->state = JobState::Stopped;
                notices.push_back("[" + std::to_string(job->id) + "] Stopped  " +
                                  job->command + "  PID: " + std::to_string(pid));
            }
        } else if (WIFCONTINUED(status)) {
            job->state = JobState::Running;
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            std::vector<pid_t>& pids = job->pids;
            pids.erase(std::remove(pids.begin(), pids.end(), pid), pids.end());
            if (pids.empty()) {
                notices.push_back("[" + std::to_string(job->id) + "] Done  " + job->command);
                erase(job->id);
            }
        }
    }
}

void JobManager::print() const {
    if (jobs_.empty()) {
        std::cout << "no active jobs\n";
        return;
    }
    for (const Job& j : jobs_) {
        std::cout << "[" << j.id << "] " << state_name(j.state) << "  " << j.command
                  << "  PID: ";
        for (size_t i = 0; i < j.pids.size(); ++i) {
            if (i) std::cout << ",";
            std::cout << j.pids[i];
        }
        std::cout << "\n";
    }
}

void JobManager::erase(int id) {
    jobs_.erase(std::remove_if(jobs_.begin(), jobs_.end(),
                               [id](const Job& j) { return j.id == id; }),
                jobs_.end());
}

// Best-effort cleanup on shell exit. The job may have exited between the last
// reap and now, so kill errors are deliberately ignored.
void JobManager::terminate_all() {
    for (const Job& j : jobs_) {
        if (j.pids.empty()) continue;
        const pid_t pgid = j.pids.front();
        if (::killpg(pgid, SIGHUP) < 0) {
            for (pid_t pid : j.pids) ::kill(pid, SIGHUP);
        }
        // A stopped job only acts on the queued SIGHUP once it runs again.
        ::killpg(pgid, SIGCONT);
    }
    jobs_.clear();
}

Job* JobManager::find(int id) {
    for (Job& j : jobs_) {
        if (j.id == id) return &j;
    }
    return nullptr;
}

Job* JobManager::find_by_pid(pid_t pid) {
    for (Job& j : jobs_) {
        if (std::find(j.pids.begin(), j.pids.end(), pid) != j.pids.end()) return &j;
    }
    return nullptr;
}
