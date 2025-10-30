#include "common.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

// Реализуйте методы:
bool Position::operator==(const Position rhs) const {
    return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
    if (row == rhs.row) {
        return col < rhs.col;
    }

    return row < rhs.row;
}

bool Position::IsValid() const {
    return (std::clamp(row, 0, MAX_ROWS - 1) == row) && std::clamp(col, 0, MAX_COLS - 1) == col;
}

namespace {
    constexpr int BASE = 26; // диапазон [A; Z]

    char NumToChar(int num) {
        return 'A' + num;
    }

    std::string ColumnToString(int num) {
        std::string result;

        do {
            int remains = num % BASE;
            result += NumToChar(remains);
            num /= BASE;
            --num;
        } while (num >= 0);

        return {result.rbegin(), result.rend()};
    }

    int CharToNum(char c) {
        return c - 'A' + 1;
    }

    int StringToColumn(std::string_view str) {
        int result = 0;

        for (auto c : str) {
            result *= BASE;
            result += CharToNum(c);
        }

        return result;
    }

} // namespace

std::string Position::ToString() const {
    if (!IsValid()) {
        return {};
    }

    return ColumnToString(col) + std::to_string(row + 1);
}

Position Position::FromString(std::string_view str) {
    const char* str_begin = str.data();
    const char* str_end = std::next(str.data(), str.size());

    const char* letters_begin = str_begin;
    const char* letters_end = std::find_if_not(str_begin, str_end, [](unsigned char c) {
        return std::isupper(c);
    });

    // Если указатели на начало и конец равны, значит букв в начале строке нет. Так же если конец букв и строки равны, значит цифр в строке нет
    if (letters_begin == letters_end || letters_end == str_end) {
        return Position::NONE;
    }

    std::string_view letters(letters_begin, std::distance(letters_begin, letters_end));

    // В теории после букв должны идти цифры, поэтому считаем, что цифры начинаются с конца стркои с буквами
    const char* nums_begin = letters_end;
    const char* nums_end = std::find_if_not(nums_begin, str_end, [](unsigned char c) {
        return std::isdigit(c);
    });

    // Если начало цифр равно концу, значит цифр в nums_begin нет, а если конец цифр не равен концу строки, значит кроме цифр еще что то есть
    if (nums_begin == nums_end || nums_end != str_end) {
        return Position::NONE;
    }

    Position result{std::atoi(nums_begin) - 1, StringToColumn(letters) - 1};

    return result.IsValid() ? result : Position::NONE;
}

bool Size::operator==(Size rhs) const {
    return cols == rhs.cols && rows == rhs.rows;
}