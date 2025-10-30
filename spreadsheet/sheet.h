#pragma once

#include "cell.h"
#include "common.h"

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;


private:
    PositionMap<Cell> cells_;               // Не пустые ячейки листа
    PositionMap<PositionSet> dependents_;   // Кто зависит от ключа

    void CacheDisability(Position pos);
    void UpdateDependencies(Position pos, const std::vector<Position>& new_refs);
};