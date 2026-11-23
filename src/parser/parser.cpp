#include "parser/parser.hpp"

#include <cctype>

namespace {

bool is_op(char c) { return c == '|' || c == '<' || c == '>' || c == '&'; }

bool is_op_token(const std::string& t) {
    return t == "|" || t == "<" || t == ">" || t == ">>" || t == "&";
}

}  // namespace

ParsedLine parse_line(const std::string& line) {
    ParsedLine out;

    // ---- tokenize: whitespace splits, quotes group, operators stand alone ----
    std::vector<std::string> tokens;
    std::string cur;
    bool in_token = false;
    char quote = 0;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (quote) {
            if (c == quote) {
                quote = 0;
            } else if (c == '\\' && quote == '"' && i + 1 < line.size() &&
                       (line[i + 1] == '"' || line[i + 1] == '\\')) {
                cur += line[++i];
            } else {
                cur += c;
            }
            continue;
        }
        if (c == '\'' || c == '"') {
            quote = c;
            in_token = true;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (in_token) {
                tokens.push_back(cur);
                cur.clear();
                in_token = false;
            }
            continue;
        }
        if (is_op(c)) {
            if (in_token) {
                tokens.push_back(cur);
                cur.clear();
                in_token = false;
            }
            if (c == '>' && i + 1 < line.size() && line[i + 1] == '>') {
                tokens.emplace_back(">>");
                ++i;
            } else {
                tokens.emplace_back(1, c);
            }
            continue;
        }
        cur += c;
        in_token = true;
    }
    if (quote) {
        out.ok = false;
        out.error = "unterminated quote";
        return out;
    }
    if (in_token) tokens.push_back(cur);

    // ---- build stages ----
    Command cmd;
    auto finish = [&]() {
        if (!cmd.program.empty()) out.stages.push_back(cmd);
        cmd = Command{};
    };

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& t = tokens[i];
        if (t == "|") {
            if (cmd.program.empty() || i + 1 >= tokens.size()) {
                out.ok = false;
                out.error = "syntax error near '|'";
                return out;
            }
            finish();
        } else if (t == "&") {
            if (i + 1 != tokens.size()) {
                out.ok = false;
                out.error = "'&' is only supported at the end of a line";
                return out;
            }
            out.background = true;
        } else if (t == "<" || t == ">" || t == ">>") {
            if (i + 1 >= tokens.size() || is_op_token(tokens[i + 1])) {
                out.ok = false;
                out.error = "missing filename after '" + t + "'";
                return out;
            }
            const std::string& file = tokens[++i];
            if (t == "<") {
                cmd.infile = file;
            } else {
                cmd.outfile = file;
                cmd.append = (t == ">>");
            }
        } else if (cmd.program.empty()) {
            cmd.program = t;
        } else {
            cmd.args.push_back(t);
        }
    }
    finish();

    // An empty line is valid input; the shell just re-prompts.
    if (out.stages.empty() && line.find_first_not_of(" \t\r\n") != std::string::npos) {
        out.ok = false;
        out.error = "empty command";
    }
    return out;
}
