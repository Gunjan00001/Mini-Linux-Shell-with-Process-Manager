#include "history/history.hpp"

#include <iostream>

void History::add(const std::string& line) {
    // Blank input is not a command, and repeating the previous command (Enter,
    // Enter) should not fill the list with duplicates.
    if (line.empty()) return;
    if (!items_.empty() && items_.back() == line) return;
    items_.push_back(line);
    // In-memory cap only; the oldest entry is dropped, numbering shifts.
    if (items_.size() > kMaxEntries) items_.erase(items_.begin());
}

void History::print() const {
    // Numbers shown here are exactly what `!N` accepts (1-based).
    for (size_t i = 0; i < items_.size(); ++i) {
        std::cout << (i + 1) << "  " << items_[i] << "\n";
    }
}

const std::string* History::at(size_t index_1based) const {
    // 0 and out-of-range ids return nullptr; the caller reports "event not
    // found" instead of dereferencing a bad index.
    if (index_1based == 0 || index_1based > items_.size()) return nullptr;
    return &items_[index_1based - 1];
}
