#include "sheet.h"

#include <algorithm>
#include <iostream>
#include <type_traits>
#include <queue>

#include "cell.h"
#include "common.h"

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    // Валидация позиции
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }

    if (auto it = cells_.find(pos); it != cells_.end()) {
        it->second.Set(std::move(text));
    } else {
        auto& new_cell = cells_.insert({pos, Cell(*this, pos)}).first->second;
        new_cell.Set(std::move(text));
    }
}

const Cell* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

Cell* Sheet::GetCell(Position pos) {
    static_assert(std::is_base_of_v<CellInterface, Cell>);

    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }

    auto it = cells_.find(pos);
    return it == cells_.end() ? nullptr : &it->second;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }

    // Ячейка полностью удаляется только в случа, если на нее никто не ссылается
    if (auto cell = cells_.find(pos); cell != cells_.end()) {
        if (!cell->second.IsReferenced()) {
            cells_.erase(cell);
        } else {
            cell->second.Set("");
        }
    }
}

Size Sheet::GetPrintableSize() const {
    if (cells_.empty()) {
        return {0, 0};
    }

    int max_row = 0;
    int max_col = 0;

    for (const auto& [pos, cell] : cells_) {
        // В таблице не могут находиться пустые ячейки, на которые никто не ссылается
        if (!cell.IsEmpty()) {
            max_row = std::max(max_row, pos.row);
            max_col = std::max(max_col, pos.col);
        }
    }

    return Size{max_row + 1, max_col + 1};
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();

    auto printer = [&output](auto&& val) {
        output << std::forward<decltype(val)>(val);
    };

    for (int i = 0; i < size.rows; ++i) {
        for (int j = 0; j < size.cols; ++j) {
            if (j > 0) {
                output << '\t';
            }

            const CellInterface* cell = GetCell(Position{i, j});
            if (cell != nullptr) {
                std::visit(printer, cell->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();

    for (int i = 0; i < size.rows; ++i) {
        for (int j = 0; j < size.cols; ++j) {
            if (j > 0) {
                output << '\t';
            }

            const CellInterface* cell = GetCell(Position{i, j});
            if (cell != nullptr) {
                output << cell->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

