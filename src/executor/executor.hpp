#pragma once

#include <sys/types.h>

#include <string>
#include <vector>

#include "history/history.hpp"
#include "parser/parser.hpp"
#include "process/job_manager.hpp"

class Executor {
public:
    // Runs one parsed line. Returns false when the shell should exit.
    bool run(const ParsedLine& line, const std::string& raw);

    JobManager& jobs() { return jobs_; }
    History& history() { return history_; }

private:
    bool run_builtin(const ParsedLine& line, bool& handled);
    void run_external(const ParsedLine& line, const std::string& raw);
    void wait_foreground(const std::vector<pid_t>& pids, const std::string& raw);

    JobManager jobs_;
    History history_;
};
