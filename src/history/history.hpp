#pragma once

#include <string>
#include <vector>

class History {
public:
    void add(const std::string& line);
    void print() const;

    // 1-based lookup for `!N` recall; nullptr when the entry does not exist.
    const std::string* at(size_t index_1based) const;

private:
    std::vector<std::string> items_;
    static constexpr size_t kMaxEntries = 1000;
};
