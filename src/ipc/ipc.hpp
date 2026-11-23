#pragma once

#include "parser/parser.hpp"

namespace ipc {

// Child side of a fork: opens the command's input/output files and dup2()s
// them onto stdin/stdout. Returns false after printing an error; caller exits.
bool apply_redirection(const Command& cmd);

}  // namespace ipc
