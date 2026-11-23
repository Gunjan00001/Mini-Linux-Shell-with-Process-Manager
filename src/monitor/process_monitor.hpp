#pragma once

#include <sys/types.h>

#include <string>
#include <vector>

namespace procmon {

// The subset of /proc/[pid]/stat we need. Field numbers in comments refer to
// proc(5); parsing starts after the last ')' so comm fields with spaces work.
struct ProcInfo {
    pid_t pid = 0;
    pid_t ppid = 0;
    char state = '?';
    std::string comm;
    unsigned long long utime = 0;   // field 14, clock ticks
    unsigned long long stime = 0;   // field 15, clock ticks
    long num_threads = 0;           // field 20
    unsigned long long vsize = 0;   // field 23, bytes
    unsigned long long rss_pages = 0;  // field 24
};

// Parses the raw contents of a stat file. Pure: unit-testable with fixtures.
bool parse_stat(const std::string& content, ProcInfo& out);

bool read_proc(pid_t pid, ProcInfo& out);
std::string read_cmdline(pid_t pid);
std::vector<ProcInfo> list_processes();

void print_monitor();                // full system table
void print_monitor_pid(pid_t pid);   // one process, detailed
void print_ps(pid_t parent);         // children of `parent`
void print_pinfo(pid_t pid);         // same detail view as monitor --pid

std::string human_size(unsigned long long bytes);

}  // namespace procmon
