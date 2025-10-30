#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <queue>

#include "sheet.h"


Cell::Cell(Sheet& sheet, Position pos) 
    : sheet_(&sheet), pos_(pos) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    using namespace std;
    
    // В этот метод не может попасть пустая строка
    if (text.empty()) {
        assert(false);
    }

    if (text.size() == 1 || text.front() != FORMULA_SIGN) {
        impl_ = make_unique<TextImpl>(move(text));
    } else {
        impl_ = make_unique<FormulaImpl>(move(text), *sheet_);
    }

    if (HasCircularDependency(pos_)) {
        throw CircularDependencyException("Cycle detected");
    }
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::CacheDisability() const {
    impl_->CacheDisability();
}

bool Cell::HasCircularDependency(Position target) const {
    PositionSet visits;
    std::queue<Position> queue;

    for (auto ref : GetReferencedCells()) {
        visits.insert(ref);
        queue.push(ref);
    }

    while (!queue.empty()) {
        Position cur_pos = queue.front();
        queue.pop();
        if (cur_pos == target) {
            return true;
        }

        auto cell = sheet_->GetCell(cur_pos);
        if (cell == nullptr) {
            continue;
        }

        for (auto ref : cell->GetReferencedCells()) {
            if (!visits.count(ref)) {
                visits.insert(ref);
                queue.push(ref);
            }
        }
    }

    return false;
}