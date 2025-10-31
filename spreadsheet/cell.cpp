#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <queue>

#include "sheet.h"


Cell::Cell(Sheet& sheet, Position pos) 
    : sheet_(&sheet), pos_(pos) {}

Cell::~Cell() = default;


namespace {
    bool IsFormula(const std::string& str) {
        return str.size() > 1 && str.front() == FORMULA_SIGN;
    }
} // namespace

void Cell::Set(std::string text) {
    if (IsFormula(text)) {
        SetFormula(std::move(text));
    } else {
        SetText(std::move(text));
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

bool Cell::IsReferenced() const {
    return !dependents_.empty();
}

bool Cell::IsEmpty() const {
    return impl_->IsEmpty();
}

void Cell::CacheDisability() const {
    PositionSet visits;
    std::queue<Position> queue;
    
    visits.insert(pos_);
    queue.push(pos_);

    while (!queue.empty()) {
        Position cur_pos = queue.front();
        queue.pop();

        Cell* cur_cell = sheet_->GetCell(cur_pos);
        if (cur_cell == nullptr) {
            continue;
        }

        cur_cell->impl_->CacheDisability();

        for (Position dependent_pos : cur_cell->dependents_) {
            if (visits.insert(dependent_pos).second) {
                queue.push(dependent_pos);
            }
        }
    }
}

bool Cell::HasCircularDependency(Position target, const FormulaImpl* formula) const {
    PositionSet visits;
    std::queue<Position> queue;

    for (auto ref : formula->GetReferencedCells()) {
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
            if (visits.insert(ref).second) {
                queue.push(ref);
            }
        }
    }

    return false;
}

void Cell::SetText(std::string str) {
    if (str.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(str));
    }

    // Инвалидация кеша, если это возможно
    CacheDisability();
}

void Cell::SetFormula(std::string formula) {
    std::unique_ptr<FormulaImpl> new_impl_ = std::make_unique<FormulaImpl>(std::move(formula), *sheet_);
    // Формулы идентичны
    if (GetText() == new_impl_->GetText()) {
        return;
    }

    // Проверка на циклы в новой формуле
    if (HasCircularDependency(pos_, new_impl_.get())) {
        throw CircularDependencyException("Cycle detected");
    }
    
    // Раз циклов нет и формулы разные, можно отвязать зависимости старой формулы и привязать зависимости новой
    UnlinkDependencies();
    impl_ = std::move(new_impl_);
    LinkDependencies();

    // Инвалидация кеша, если это возможно
    CacheDisability();
}

void Cell::UnlinkDependencies() {
    for (auto ref : GetReferencedCells()) {
        Cell* cell = sheet_->GetCell(ref);
        cell->dependents_.erase(pos_);
    }
}

void Cell::LinkDependencies() {
    for (auto ref : GetReferencedCells()) {
        // Если пустой ячейке в таблице не существовало, чтобы на нее суметь сослаться, ее надо создать
        if (sheet_->GetCell(ref) == nullptr) {
            sheet_->SetCell(ref, "");
        }

        Cell* cell = sheet_->GetCell(ref);
        cell->dependents_.insert(pos_);
    }
}