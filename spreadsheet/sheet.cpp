#include "sheet.h"

#include <algorithm>
#include <iostream>
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

    // Обработка пустого текста
    if (text.empty()) {
        ClearCell(pos);
        return;
    }

    Cell new_cell(*this, pos);
    new_cell.Set(std::move(text));

    if (auto old_cell = cells_.find(pos); old_cell != cells_.end()) {
        // Одинаковые данные не нуждаются в перезаписи
        if (new_cell.GetText() == old_cell->second.GetText()) {
            return;
        }
    }

    CacheDisability(pos);
    UpdateDependencies(pos, new_cell.GetReferencedCells());
    
    if (auto it = cells_.find(pos); it != cells_.end()) {
        it->second = std::move(new_cell);
    } else {
        cells_.insert({pos, std::move(new_cell)});
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
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

    // Ячейка полностью удаляется, становясь пустой
    if (auto it = cells_.find(pos); it != cells_.end()) {
        cells_.erase(it);
    }

    // Инвалидация хранимого вычисления в кеше зависимых ячеек
    CacheDisability(pos);
    UpdateDependencies(pos, {});
}

Size Sheet::GetPrintableSize() const {
    if (cells_.empty()) {
        return {0, 0};
    }

    int max_row = 0;
    int max_col = 0;

    for (const auto& [pos, cell] : cells_) {
        max_row = std::max(max_row, pos.row);
        max_col = std::max(max_col, pos.col);
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

void Sheet::CacheDisability(Position pos) {
    PositionSet visits;
    std::queue<Position> queue;
    
    visits.insert(pos);
    queue.push(pos);

    while (!queue.empty()) {
        Position cur_pos = queue.front();
        queue.pop();

        if (auto cur_cell = cells_.find(cur_pos); cur_cell != cells_.end()) {
            cur_cell->second.CacheDisability();
        }

        for (Position ref : dependents_[cur_pos]) {
            if (!visits.count(ref)) {
                visits.insert(ref);
                queue.push(ref);
            }
        }
    }
}

void Sheet::UpdateDependencies(Position pos, const::std::vector<Position>& new_refs) {
    // 1 раз сохраняю ссылку на множество, чтобы на каждой итерации не пересчитывать хэш и не искать множество в словаре
    CellInterface* cell = GetCell(pos);
    std::vector<Position> dependencies = cell != nullptr
                                        ? cell->GetReferencedCells()
                                        : std::vector<Position>();
    
    // Удаляется информация о том, что cell ссылался на old_ref
    for (auto old_ref : dependencies) {
        dependents_[old_ref].erase(pos);
    }
    
    // Добавляется информация о том, что cell ссылается на new_ref
    for (auto new_ref : new_refs) {
        dependents_[new_ref].insert(pos);
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

