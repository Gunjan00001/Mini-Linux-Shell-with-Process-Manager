#pragma once

#include <string>
#include <vector>

// One stage of a pipeline: program + args + optional redirections.
struct Command {
    std::string program;
    std::vector<std::string> args;
    std::string infile;   // empty = no input redirection
    std::string outfile;  // empty = no output redirection
    bool append = false;  // >> instead of >
};

struct ParsedLine {
    std::vector<Command> stages;  // one per pipeline stage
    bool background = false;      // trailing '&'
    bool ok = true;
    std::string error;
};

// Tokenizes and parses one input line. Pure function: never touches the OS.
ParsedLine parse_line(const std::string& line);
