#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

FormulaAST TryParseFormulaAST(std::string expression) {
    try {
        return ParseFormulaAST(std::move(expression));
    } catch (const std::exception& e) {
        throw FormulaException("Incorrect formula\n"s + e.what());
    }
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression)
        : ast_(TryParseFormulaAST(std::move(expression))) {}

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        } catch (const FormulaError& e) {
            return e;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream oss;
        ast_.PrintFormula(oss);
        return oss.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        const auto& ref_list = ast_.GetCells();
        //Список очищается от дубликатов и сортируется
        std::set<Position> references(ref_list.begin(), ref_list.end());
        // Формируется вектор
        return {references.begin(), references.end()};
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}