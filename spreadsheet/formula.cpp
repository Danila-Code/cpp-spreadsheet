#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << '#' << fe.ToString() << '!';
}

FormulaError::FormulaError(FormulaError::Category category) : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError other) const {
    return category_ == other.category_;
}

std::string_view FormulaError::ToString() const {
    if (category_ == Category::Ref) {
        return "REF"sv;
    } else if (category_ == Category::Value) {
        return "VALUE"sv;
    } else {
        return "ARITHM"sv;
    }
}

namespace {
struct FromCellValueToDouble {
    double operator()(double num) {
        return num;
    }
    double operator()(const std::string& text) {
        if (text.empty()) {
            return 0.0;
        }
        std::istringstream str_to_double(text);
        double num;
        str_to_double >> num;

        if(!str_to_double.eof() || str_to_double.fail()) {
            throw FormulaError(FormulaError::Category::Value);
        }
        return num;
    }
    double operator()(const FormulaError& e) {
        throw e;
    }
};


class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression)
        : ast_(ParseFormulaAST(std::move(expression))) {
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        std::function<double(const Position)> get_value = [&sheet](const Position pos) {
            if (!pos.IsValid()) {
                 throw FormulaError(FormulaError::Category::Ref);
            }
            auto p_cell = sheet.GetCell(pos);
            if(!p_cell) {
                return 0.0;
            }
            auto cell_value = p_cell->GetValue();
            return std::visit(FromCellValueToDouble(), cell_value);
        };
        try {
            return ast_.Execute(get_value);
        } catch (const FormulaError& e) {
            return e;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out_str;
        ast_.PrintFormula(out_str);
        return out_str.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        auto cell_list = ast_.GetCells();
        cell_list.unique();
        return std::vector<Position>(cell_list.begin(), cell_list.end());
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}