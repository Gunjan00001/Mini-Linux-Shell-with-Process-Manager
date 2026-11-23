#include "monitor/process_monitor.hpp"

#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace procmon {

namespace {

long page_size() { return ::sysconf(_SC_PAGESIZE); }
long clock_ticks() { return ::sysconf(_SC_CLK_TCK); }

std::string proc_path(pid_t pid, const char* file) {
    return "/proc/" + std::to_string(pid) + "/" + file;
}

std::string trim_comm(const std::string& s) {
    return s.size() > 24 ? s.substr(0, 24) : s;
}

void system_summary() {
    unsigned long long total_kb = 0, avail_kb = 0;
    std::ifstream meminfo("/proc/meminfo");
    std::string key, unit;
    unsigned long long val = 0;
    while (meminfo >> key >> val >> unit) {
        if (key == "MemTotal:") total_kb = val;
        if (key == "MemAvailable:") avail_kb = val;
    }
    double uptime = 0.0;
    std::ifstream uptime_file("/proc/uptime");
    uptime_file >> uptime;

    std::cout << "Tasks: " << list_processes().size()
              << "   Uptime: " << static_cast<unsigned long long>(uptime) << "s"
              << "   Mem: " << human_size((total_kb - avail_kb) * 1024ULL) << " / "
              << human_size(total_kb * 1024ULL) << "\n\n";
}

void print_detail(pid_t pid, const std::string& title) {
    ProcInfo p;
    if (!read_proc(pid, p)) {
        std::cerr << "minishell: no such process: " << pid << "\n";
        return;
    }

    // Utilization needs two samples; 200 ms is short enough to feel instant and
    // long enough for a tick to land on any reasonable CLK_TCK.
    constexpr useconds_t kSampleUs = 200 * 1000;
    ProcInfo later;
    ::usleep(kSampleUs);
    const bool sampled = read_proc(pid, later);

    const double secs = static_cast<double>(p.utime + p.stime) / clock_ticks();
    const double user = static_cast<double>(p.utime) / clock_ticks();

    std::cout << "========== " << title << ": " << pid << " ==========\n"
              << "PID:        " << p.pid << "\n"
              << "PPID:       " << p.ppid << "\n"
              << "State:      " << p.state << "\n"
              << "Threads:    " << p.num_threads << "\n"
              << "CPU time:   " << std::fixed << std::setprecision(2) << secs
              << " s (user " << user << " s)\n";
    if (sampled && clock_ticks() > 0) {
        long long delta = static_cast<long long>(later.utime + later.stime) -
                          static_cast<long long>(p.utime + p.stime);
        if (delta < 0) delta = 0;
        const double pct =
            100.0 * static_cast<double>(delta) / (clock_ticks() * (kSampleUs / 1e6));
        std::cout << "CPU now:    " << std::fixed << std::setprecision(1) << pct
                  << " % (200 ms sample)\n";
    }
    std::cout << "Virtual:    " << human_size(p.vsize) << "\n"
              << "Resident:   " << human_size(p.rss_pages * static_cast<unsigned long long>(page_size()))
              << "\n"
              << "Name:       " << p.comm << "\n"
              << "Cmdline:    " << read_cmdline(pid) << "\n";
    std::cout.unsetf(std::ios::fixed);
}

}  // namespace

bool parse_stat(const std::string& s, ProcInfo& out) {
    const size_t open = s.find('(');
    const size_t close = s.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close < open) return false;

    out.comm = s.substr(open + 1, close - open - 1);

    std::istringstream in(s.substr(close + 1));
    long long ppid = 0;
    if (!(in >> out.state >> ppid)) return false;
    out.ppid = static_cast<pid_t>(ppid);

    unsigned long long skip = 0;
    for (int i = 0; i < 9; ++i) in >> skip;  // fields 5..13
    in >> out.utime >> out.stime;            // 14, 15
    for (int i = 0; i < 4; ++i) in >> skip;  // 16..19
    in >> out.num_threads;                   // 20
    in >> skip >> skip;                      // 21, 22
    in >> out.vsize >> out.rss_pages;        // 23, 24
    return static_cast<bool>(in);
}

bool read_proc(pid_t pid, ProcInfo& out) {
    std::ifstream f(proc_path(pid, "stat"));
    if (!f) return false;
    std::stringstream buf;
    buf << f.rdbuf();
    out.pid = pid;
    return parse_stat(buf.str(), out);
}

std::string read_cmdline(pid_t pid) {
    std::ifstream f(proc_path(pid, "cmdline"), std::ios::binary);
    if (!f) return "";
    std::string raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::replace(raw.begin(), raw.end(), '\0', ' ');
    while (!raw.empty() && raw.back() == ' ') raw.pop_back();
    return raw;
}

std::vector<ProcInfo> list_processes() {
    std::vector<ProcInfo> result;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator("/proc", ec)) {
        const std::string name = entry.path().filename().string();
        if (name.empty() || !std::all_of(name.begin(), name.end(), ::isdigit)) continue;
        ProcInfo p;
        if (read_proc(static_cast<pid_t>(std::stol(name)), p)) result.push_back(p);
    }
    std::sort(result.begin(), result.end(),
              [](const ProcInfo& a, const ProcInfo& b) { return a.pid < b.pid; });
    return result;
}

void print_monitor() {
    std::cout << "================ MINI SHELL MONITOR ================\n\n";
    system_summary();
    std::cout << std::left << std::setw(8) << "PID" << std::setw(8) << "PPID" << std::setw(10)
              << "STATE" << std::setw(6) << "THR" << std::setw(12) << "RSS" << "COMMAND\n";
    std::cout << "-----------------------------------------------------\n";
    for (const ProcInfo& p : list_processes()) {
        std::cout << std::left << std::setw(8) << p.pid << std::setw(8) << p.ppid
                  << std::setw(10) << p.state << std::setw(6) << p.num_threads
                  << std::setw(12)
                  << human_size(p.rss_pages * static_cast<unsigned long long>(page_size()))
                  << trim_comm(p.comm) << "\n";
    }
    std::cout << "=====================================================\n";
}

void print_monitor_pid(pid_t pid) { print_detail(pid, "PROCESS INFO"); }

void print_pinfo(pid_t pid) { print_detail(pid, "PINFO"); }

void print_ps(pid_t parent) {
    std::cout << std::left << std::setw(8) << "PID" << std::setw(8) << "PPID" << std::setw(8)
              << "STATE" << "COMMAND\n";
    std::cout << "------------------------------------------\n";
    bool any = false;
    for (const ProcInfo& p : list_processes()) {
        if (p.ppid != parent) continue;
        any = true;
        std::cout << std::left << std::setw(8) << p.pid << std::setw(8) << p.ppid
                  << std::setw(8) << p.state << trim_comm(p.comm) << "\n";
    }
    if (!any) std::cout << "(no child processes)\n";
}

std::string human_size(unsigned long long bytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 4) {
        v /= 1024.0;
        ++u;
    }
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.1f %s", v, units[u]);
    return buf;
}

}  // namespace procmon
