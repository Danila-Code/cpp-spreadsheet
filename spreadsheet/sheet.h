#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

struct PositionHasher {
    std::size_t operator()(const Position& pos) const {
        // each of the fields (row and col) in Position does not exceed 16383 and takes up no more than two bytes
        return std::hash<int>{}((pos.col << 16) + pos.row);
    }
};

class Sheet : public SheetInterface {
public:
    using Table = std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher>;

    ~Sheet();
    // sets the contents of the cell if the pos position is valid
    void SetCell(Position pos, std::string text) override;

    // returns a const pointer to the CellInterface with position pos, if it is empty returns nullptr
    const CellInterface* GetCell(Position pos) const override;
    // returns a pointer to the CellInterface with position pos, if it is empty returns nullptr
    CellInterface* GetCell(Position pos) override;

    // clears the contents of a cell or deletes a cell if it is not connected to other cells
    void ClearCell(Position pos) override;
    // returns the size of the minimum rectangular area of the table
    Size GetPrintableSize() const override;

    // outputs cell values — strings, numbers, or FormulaError
    void PrintValues(std::ostream& output) const override;
    // outputs text representations of cells
    void PrintTexts(std::ostream& output) const override;
    // return pointer to Cell
    Cell* GetCellPtr(Position pos) const;

private:
    // print table
    template <typename PrintFunc>
    void Print(std::ostream& output, PrintFunc func) const;
    
    Table table_{};
};