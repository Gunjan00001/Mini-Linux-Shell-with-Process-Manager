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

const std::string* History::at(size_t index_1based) const {
    if (index_1based == 0 || index_1based > items_.size()) return nullptr;
    return &items_[index_1based - 1];
}
