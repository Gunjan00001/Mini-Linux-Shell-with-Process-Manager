#include "history/history.hpp"

#include <iostream>

void History::add(const std::string& line) {
    if (line.empty()) return;
    if (!items_.empty() && items_.back() == line) return;
    items_.push_back(line);
    if (items_.size() > kMaxEntries) items_.erase(items_.begin());
}

void History::print() const {
    for (size_t i = 0; i < items_.size(); ++i) {
        std::cout << (i + 1) << "  " << items_[i] << "\n";
    }
}
