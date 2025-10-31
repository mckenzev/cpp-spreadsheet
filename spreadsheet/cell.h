#pragma once

#include <memory>
#include <optional>

#include "common.h"
#include "formula.h"

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Cell&&) = default;
    Cell(Sheet& sheet, Position pos);
    ~Cell();
    Cell& operator=(Cell&&) = default;

    void Set(std::string text);

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    bool IsReferenced() const;
    bool IsEmpty() const;

private:
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual void CacheDisability() const = 0;
        virtual bool IsEmpty() const = 0;
    };

    class EmptyImpl : public Impl {
    public:
        Value GetValue() const override {
            return std::string();
        };

        std::string GetText() const override {
            return "";
        }

        std::vector<Position> GetReferencedCells() const override {
            return {};
        }

        void CacheDisability() const override {}

        bool IsEmpty() const override {
            return true;
        }

    private:
        std::string data_;
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string str) : data_(std::move(str)) {}

        Value GetValue() const override {
            return data_.front() != ESCAPE_SIGN ? data_ : data_.substr(1);
        };

        std::string GetText() const override {
            return data_;
        }

        std::vector<Position> GetReferencedCells() const override {
            return {};
        }

        void CacheDisability() const override {}

        bool IsEmpty() const override {
            return false;
        }

    private:
        std::string data_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string str, const SheetInterface& sheet)
        : formula_(ParseFormula(str.substr(1))),
          sheet_(sheet) {} // FormulaImpl формируется из формульной строки, которая длинее 1 символа и начинается с '='

        Value GetValue() const override {
            if (!cache_.has_value()) {
                auto val = std::visit([this](auto&& res) -> Value {
                    return std::forward<decltype(res)>(res);
                }, formula_->Evaluate(this->sheet_));
                
                cache_.emplace(std::move(val));
            }

            return *cache_;
        };

        std::string GetText() const override {
            return '=' + formula_->GetExpression();
        }

        std::vector<Position> GetReferencedCells() const override {
            return formula_->GetReferencedCells();
        }

        void CacheDisability() const override {
            cache_.reset();
        }

        bool IsEmpty() const override {
            return false;
        }

    private:
        std::unique_ptr<FormulaInterface> formula_;
        const SheetInterface& sheet_;
        mutable std::optional<Value> cache_; // Закешированное значение, возвращаемое методом GetValue()
    };

    void SetText(std::string str);
    void SetFormula(std::string formula);
    bool HasCircularDependency(Position target, const FormulaImpl* formula) const;
    void CacheDisability() const;
    void UnlinkDependencies();
    void LinkDependencies();

    std::unique_ptr<Impl> impl_ = std::make_unique<EmptyImpl>();
    Sheet* sheet_; // С хранением указателя вместо ссылки становится доступен перемещающий оператор и конструктор
    Position pos_;
    PositionSet dependents_;
};