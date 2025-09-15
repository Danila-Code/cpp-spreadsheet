#pragma once

#include "common.h"

#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;
    // checks whether other cells refer to this one
    bool IsReferenced() const;

private:
    // types of using cells
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    // creates a cell if it does not exist in position pos and return pointer to Cell
    const Cell* GetInitializeCell(Position pos) const;

    // the method updates the links to dependent and referenced cells
    void UpdateDependencies();
    // the method removes the dependency between this cell and the others
    void RemoveOldDependencies() const;
    // the method adds a dependency between this cell and the referenced
    void AddNewDependencies() const;
    // the method clear referenced_cells and add new referenced cells
    void UpdateReferencedCells();

    // the method determines the presence of cyclic dependence in the formula
    bool HasCyclicDependence(const Impl* new_impl) const;

    // the method invalidates cashed values in the cells
    void InvalidateCache() const;
    // this method recursively invalidates values cached in cells
    void InvalidateCachedValue(std::unordered_set<const Cell*> visited) const;

    // fields
    Sheet& sheet_;
    // content of the cell
    std::unique_ptr<Impl> impl_;

    // cells that depends from this cell
    mutable std::unordered_set<const Cell*> dependent_cells_;
    // the cells referenced by this cell
    std::unordered_set<const Cell*> referenced_cells;
};