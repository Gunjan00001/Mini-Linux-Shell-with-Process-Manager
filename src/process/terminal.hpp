#pragma once

#include <sys/types.h>

// Terminal ownership for job control. When stdin is a tty we put the shell in
// its own process group and hand the terminal to the foreground job, so Ctrl+C
// and Ctrl+Z reach only that job. Without a tty (pipes, CI) every function is a
// no-op and children simply inherit the shell's process group.
namespace terminal {

bool init();  // call once, before the first fork
bool active();
pid_t shell_pgid();
void give_to(pid_t pgid);
void take_back();

}  // namespace terminal
