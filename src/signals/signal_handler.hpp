#pragma once

namespace signals {

// Installs the shell's handlers. SIGCHLD (and SIGINT) are delivered to the
// shell as a byte written to a self-pipe; the main loop reads that pipe.
void install();

// Read end of the self-pipe (blocking select target). -1 if setup failed.
int notify_fd();

// Drains pending signal bytes (non-blocking).
void drain_notifications();

// Restores default signal dispositions and closes pipe fds. Called in the
// child right after fork(), before execvp().
void reset_in_child();

}  // namespace signals
