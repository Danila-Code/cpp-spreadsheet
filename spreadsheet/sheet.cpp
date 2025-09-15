#include "sheet.h"

#include <algorithm>
#include <iostream>

using namespace std::literals;

Sheet::~Sheet() {}

// return pointer to Cell
Cell* Sheet::GetCellPtr(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    auto iter = table_.find(pos);
    if(iter == table_.end()) {
        return nullptr;
    }
    const auto& p_cell = iter->second;
    return p_cell.get();
}

// sets the contents of the cell if the pos position is valid
void Sheet::SetCell(Position pos, std::string text) {
    if (Cell* cell = GetCellPtr(pos); cell) {
        cell->Set(std::move(text));
    } else {
        auto new_cell = table_.emplace(pos, new Cell(*this));
        new_cell.first->second->Set(std::move(text));
    }
}

// returns a pointer to the CellInterface with position pos, if it is empty returns nullptr
CellInterface* Sheet::GetCell(Position pos) {
    return GetCellPtr(pos);
}

// returns a const pointer to the CellInterface with position pos, if it is empty returns nullptr
const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

// clears the contents of a cell or deletes a cell if it is not connected to other cells
void Sheet::ClearCell(Position pos) {
    Cell* cell = GetCellPtr(pos);
    if (!cell) {
        return;
    }
    // if any cell depends on the cell being cleared, we only
    // clear the contents and do not delete the cell
    if(cell->IsReferenced()) {
        cell->Set({});
        return;
    }
    table_.erase(pos);
}

// returns the size of the minimum rectangular area of the table
Size Sheet::GetPrintableSize() const {
    if (!table_.size()) {
        return {0, 0};
    }
    Size printable_size{0, 0};
    std::for_each(table_.begin(), table_.end(),
        [&printable_size](const auto& cell) {
            printable_size.rows = std::max(cell.first.row, printable_size.rows);
            printable_size.cols = std::max(cell.first.col, printable_size.cols);
    });
    ++printable_size.rows;
    ++printable_size.cols;
    return printable_size;
}

// print table
template <typename PrintFunc>
void Sheet::Print(std::ostream& output, PrintFunc func) const {
    Size printable_size = GetPrintableSize();
    for (int i = 0; i < printable_size.rows; ++i) {
        for (int j = 0; j < printable_size.cols; ++j) {
            if (j != 0) {
                output << '\t';
            }
            const auto iter = table_.find(Position{i, j});
            if (iter != table_.end()) {
                const auto& p_cell = iter->second;
                func(p_cell);
            }
        }
        output << std::endl;
    }
}

// outputs cell values — strings, numbers, or FormulaError
void Sheet::PrintValues(std::ostream& output) const {
    Print(output, [&output](const std::unique_ptr<Cell>& cell) {
        std::visit([&output](const auto&& value) {
            output << value;
        }, cell->GetValue());
    });
}

// outputs text representations of cells
void Sheet::PrintTexts(std::ostream& output) const {
    Print(output, [&output](const std::unique_ptr<Cell>& cell) {
        output << cell->GetText();
    });
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}