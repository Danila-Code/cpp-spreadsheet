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
    std::string_view col_str;
    std::string_view row_str;

    auto is_valid_letter = [](char c) {
        return c >= FIRST_LETTER && c < FIRST_LETTER + LETTERS;
    };

    size_t num_begin = 0;
    for (size_t i = 0; i < str.size(); ++i) {
        if (num_begin && !isdigit(str[i])) {
            return NONE;
        }
        if (isdigit(str[i]) && !num_begin) {
            num_begin = i;
            continue;
        }
        if (!num_begin && !is_valid_letter(str[i])) {
            return NONE;
        }
    }
    row_str = str.substr(num_begin);
    col_str = str.substr(0, num_begin);

    Position pos{std::atoi(row_str.data()) - 1,
                 FromLatinAlphaToDecimal(col_str)};
    return pos.IsValid() ? pos : NONE;
}