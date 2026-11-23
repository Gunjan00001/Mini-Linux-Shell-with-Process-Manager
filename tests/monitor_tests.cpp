// Assert-based /proc parser tests: fixture parsing plus a live self-check.
#include "monitor/process_monitor.hpp"

#include <unistd.h>

#ifdef NDEBUG
#undef NDEBUG  // tests must assert even in Release builds
#endif
#include <cassert>
#include <iostream>

namespace {

const char* kFixture =
    "4321 (my prog) S 1000 4321 4321 0 -1 4194560 100 0 0 0 "
    "11 7 0 0 20 0 3 0 500 1000000 250";

void fixture_fields() {
    procmon::ProcInfo p;
    assert(procmon::parse_stat(kFixture, p));
    assert(p.comm == "my prog");  // comm containing a space must survive
    assert(p.state == 'S');
    assert(p.ppid == 1000);
    assert(p.utime == 11);
    assert(p.stime == 7);
    assert(p.num_threads == 3);
    assert(p.vsize == 1000000);
    assert(p.rss_pages == 250);
}

void live_self() {
    procmon::ProcInfo p;
    assert(procmon::read_proc(::getpid(), p));
    assert(p.pid == ::getpid());
    assert(p.ppid > 0);
    assert(!p.comm.empty());
    assert(!procmon::read_cmdline(::getpid()).empty());
}

void bad_input() {
    procmon::ProcInfo p;
    assert(!procmon::parse_stat("garbage", p));
    assert(!procmon::read_proc(1 << 22, p));  // pid 4M: does not exist
}

void listing() {
    assert(!procmon::list_processes().empty());
    assert(procmon::human_size(2048) == "2.0 KB");
}

}  // namespace

int main() {
    fixture_fields();
    live_self();
    bad_input();
    listing();
    std::cout << "monitor tests passed\n";
    return 0;
}
