#pragma once

#include <string>
#include <vector>

class History {
public:
    void add(const std::string& line);
    void print() const;

private:
    std::vector<std::string> items_;
    static constexpr size_t kMaxEntries = 1000;
};
