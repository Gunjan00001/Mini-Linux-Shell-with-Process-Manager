// Assert-based parser tests: no framework, run as `parser_tests`.
#include "parser/parser.hpp"

#ifdef NDEBUG
#undef NDEBUG  // tests must assert even in Release builds
#endif
#include <cassert>
#include <iostream>

namespace {

void simple_command() {
    ParsedLine p = parse_line("ls -la");
    assert(p.ok);
    assert(p.stages.size() == 1);
    assert(p.stages[0].program == "ls");
    assert(p.stages[0].args.size() == 1);
    assert(p.stages[0].args[0] == "-la");
    assert(!p.background);
}

void quoted_arguments() {
    ParsedLine p = parse_line("echo \"Hello World\" 'a b'");
    assert(p.ok);
    assert(p.stages[0].program == "echo");
    assert(p.stages[0].args.size() == 2);
    assert(p.stages[0].args[0] == "Hello World");
    assert(p.stages[0].args[1] == "a b");
}

void extra_whitespace() {
    ParsedLine p = parse_line("   ls    -l   ");
    assert(p.ok);
    assert(p.stages[0].program == "ls");
    assert(p.stages[0].args.size() == 1);
}

void background() {
    ParsedLine p = parse_line("sleep 30 &");
    assert(p.ok);
    assert(p.background);
    assert(p.stages[0].program == "sleep");
}

void pipeline() {
    ParsedLine p = parse_line("ls -l | grep txt");
    assert(p.ok);
    assert(p.stages.size() == 2);
    assert(p.stages[0].program == "ls");
    assert(p.stages[1].program == "grep");
    assert(p.stages[1].args[0] == "txt");
}

void redirection() {
    ParsedLine p = parse_line("ls > out.txt");
    assert(p.ok);
    assert(p.stages[0].outfile == "out.txt");
    assert(!p.stages[0].append);

    ParsedLine q = parse_line("cat < in.txt");
    assert(q.ok);
    assert(q.stages[0].infile == "in.txt");

    ParsedLine r = parse_line("echo hi >> log.txt");
    assert(r.ok);
    assert(r.stages[0].outfile == "log.txt");
    assert(r.stages[0].append);
}

void redirection_without_file() {
    ParsedLine p = parse_line("ls >");
    assert(!p.ok);
}

void dangling_pipe() {
    ParsedLine p = parse_line("ls |");
    assert(!p.ok);
}

void unterminated_quote() {
    ParsedLine p = parse_line("echo \"oops");
    assert(!p.ok);
}

void empty_line_is_valid() {
    ParsedLine p = parse_line("   ");
    assert(p.ok);
    assert(p.stages.empty());
}

void amp_mid_line_rejected() {
    ParsedLine p = parse_line("sleep 1 & echo hi");
    assert(!p.ok);
}

}  // namespace

int main() {
    simple_command();
    quoted_arguments();
    extra_whitespace();
    background();
    pipeline();
    redirection();
    redirection_without_file();
    dangling_pipe();
    unterminated_quote();
    empty_line_is_valid();
    amp_mid_line_rejected();
    std::cout << "parser tests passed\n";
    return 0;
}
