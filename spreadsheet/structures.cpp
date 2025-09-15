#include "common.h"

#include <cctype>
#include <regex>
#include <sstream>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const char FIRST_LETTER = 'A';

const Position Position::NONE = {-1, -1};

namespace {
// converts a number from the decimal system to a system of calculus based on the Latin alphabet
std::string FromDecimalToLatinAlpha(int decimal_num) {
    std::string res;
    res.reserve(MAX_POSITION_LENGTH);
    do {
        res.push_back(FIRST_LETTER + (decimal_num - 1) % LETTERS);
        decimal_num = (decimal_num - 1) / LETTERS;
    } while (decimal_num);
    return {res.rbegin(), res.rend()};
}
// converts a string of Latin letters to a decimal number
int FromLatinAlphaToDecimal(std::string_view latin_str) {
    int decimal_num = 0;
    int pos_pow = 1;
    for (int i = latin_str.size() - 1; i >= 0; --i) {
        decimal_num += (latin_str[i] - FIRST_LETTER + 1) * pos_pow;
        pos_pow *= LETTERS;
    }
    return --decimal_num;
}
}  // namespace

bool Position::operator==(const Position rhs) const {
    return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
    return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
    return row >= 0 && row < MAX_ROWS && col >= 0 && col < MAX_COLS;
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return {};
    }
    return FromDecimalToLatinAlpha(col + 1) + std::to_string(row + 1);
}

bool Size::operator==(Size rhs) const {
    return rows == rhs.rows && cols == rhs.cols;
}

Position Position::FromString(std::string_view str) {
    std::regex row_col_regex(R"/(([A-Z]+)([0-9]+))/");
    std::cmatch match_res;

    if(!std::regex_match(str.data(), match_res, row_col_regex)) {
        return NONE;
    }

    Position pos{std::atoi(match_res[2].str().data()) - 1,
                 FromLatinAlphaToDecimal(match_res[1].str())};
    return pos.IsValid() ? pos : NONE;
}