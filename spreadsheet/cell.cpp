#include "cell.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <deque>

namespace {
void AddCellsToDeque(const Sheet& sheet, const std::vector<Position>& positions, std::deque<const Cell*>& pointers) {
    for (Position pos : positions) {
        auto p_cell = sheet.GetCellPtr(pos);
        if(p_cell) {
            pointers.push_back(p_cell);
        }
    }
}
}  // namespace

// base class for cells content
class Cell::Impl {
public:
    virtual std::string GetText() const = 0;
    virtual Value GetValue() const = 0;
    virtual std::vector<Position> GetReferencedCells() const {
        return {};
    };
    virtual void InvalidateCachedValue() const {
    }
};

// class of empty cell
class Cell::EmptyImpl : public Cell::Impl {
public:
    std::string GetText() const override {
        return {};
    }
    Value GetValue() const override {
        return {};
    }
};

// class of cell with text
class Cell::TextImpl : public Cell::Impl {
public:
    explicit TextImpl(std::string text) : text_{std::move(text)} {
    }

    std::string GetText() const override {
        return text_;
    }

    Value GetValue() const override {
        return text_[0] == ESCAPE_SIGN ? text_.substr(1) : text_;
    }
private:
    std::string text_;
};

// class of cell with formula
class Cell::FormulaImpl : public Cell::Impl {
public:
    explicit FormulaImpl(Sheet& sheet, std::string expression)
            : sheet_{sheet}, formula_{ParseFormula(expression.substr(1))} {

    }

    std::string GetText() const override {
        return '=' + formula_->GetExpression();
    }

    Value GetValue() const override {
        if(!cached_value_.has_value()) {
            cached_value_ = formula_->Evaluate(sheet_);
        }

        if(std::holds_alternative<double>(cached_value_.value())) {
            return std::get<double>(cached_value_.value());
        } else {
            return std::get<FormulaError>(cached_value_.value());
        }
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

    void InvalidateCachedValue() const override {
        cached_value_.reset();
    }

private:
    Sheet& sheet_;
    std::unique_ptr<FormulaInterface> formula_;

    mutable std::optional<FormulaInterface::Value> cached_value_;
};

// class Cell methods
Cell::Cell(Sheet& sheet) : sheet_{sheet}, impl_(new EmptyImpl) {
}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    if(impl_->GetText() == text) {
        return;
    }

    std::unique_ptr<Impl> new_impl;
    if(text.empty()) {
        new_impl = std::make_unique<EmptyImpl>();
    } else if(text[0] == '=' && text.size() > 1) {
        new_impl = std::make_unique<FormulaImpl>(sheet_, std::move(text));
        // after parsing the formula, the extra brackets can be removed
        // and texts may be equal
        if(new_impl->GetText() == impl_->GetText()) {
            return;
        }
        if(HasCyclicDependence(new_impl.get())) {
            throw CircularDependencyException("Formula has circular dependence");
        }
    } else {
        new_impl = std::make_unique<TextImpl>(std::move(text));
    }
    impl_ = std::move(new_impl);

    UpdateDependencies();
    InvalidateCache();
}

void Cell::Clear() {
    impl_.reset(new EmptyImpl);
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

// the method determines the presence of cyclic dependence in the formula
bool Cell::HasCyclicDependence(const Impl* new_impl) const {
    if(new_impl->GetReferencedCells().empty()) {
        return false;
    }

    std::deque<const Cell*> to_visit;
    AddCellsToDeque(sheet_, new_impl->GetReferencedCells(), to_visit);

    std::unordered_set<const Cell*> visited;
 
    while (!to_visit.empty()) {
        auto current_cell = to_visit.front();
        to_visit.pop_front();

        if(current_cell == this) {
            return true;
        }
        if(visited.count(current_cell)) {
            continue;
        }
        visited.insert(current_cell);
        AddCellsToDeque(sheet_, current_cell->GetReferencedCells(), to_visit);
    }
    return false;
}

// the method invalidates cashed values in the cells
void Cell::InvalidateCache() const {
    if (!dependent_cells_.empty()) {
        std::unordered_set<const Cell*> visited;
        InvalidateCachedValue(visited);
    }
}

// this method recursively invalidates values cached in cells
void Cell::InvalidateCachedValue(std::unordered_set<const Cell*> visited) const {
    impl_->InvalidateCachedValue();
    for (const auto p_cell : dependent_cells_) {
        if(visited.count(p_cell)) {
            continue;
        }
        visited.insert(p_cell);
        p_cell->InvalidateCachedValue(visited);
    }
}

// the method updates the links to dependent and referenced cells
void Cell::UpdateDependencies() {
    RemoveOldDependencies();
    UpdateReferencedCells();
    AddNewDependencies();
}

// the method removes the dependency between this cell and the others
void Cell::RemoveOldDependencies() const {
    if (referenced_cells.empty()) {
        return;
    }
    for (auto p_cell : referenced_cells) {
        p_cell->dependent_cells_.erase(this);
    }
}

// the method adds a dependency between this cell and the referenced
void Cell::AddNewDependencies() const {
    for (auto p_cell : referenced_cells) {
        p_cell->dependent_cells_.insert(this);
    }
}

// the method clear referenced_cells and add new referenced cells
void Cell::UpdateReferencedCells() {
    referenced_cells.clear();
    for(Position pos : impl_->GetReferencedCells()) {
        const Cell* p_cell = GetInitializeCell(pos);
        assert(p_cell);
        referenced_cells.insert(p_cell);
    }
}

// creates a cell if it does not exist in position pos and return pointer to Cell
const Cell* Cell::GetInitializeCell(Position pos) const {
    if (const Cell* cell = sheet_.GetCellPtr(pos); !cell) {
        sheet_.SetCell(pos, {});
    }
    return sheet_.GetCellPtr(pos);
}

// checks whether other cells refer to this one
bool Cell::IsReferenced() const {
    return !dependent_cells_.empty();
}