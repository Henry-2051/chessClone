#include <format>
#include <string>
#include <utility>
#include <iostream>

std::pair<std::string, std::string> split_by_delimiter(std::string input, char delim) {
    std::string newstring;

    size_t first_delim_idx = input.find(delim);

    if (first_delim_idx == std::string::npos) {
        return {"", input};
    }

    newstring = input.substr(0, first_delim_idx);

    input = input.substr(first_delim_idx + 1);

    return {newstring, input};
}

void printpair(std::pair<std::string, std::string> val) {
    std::cout << std::format("[\"{}\", \"{}\"]\n", val.first, val.second);
}

int main (int argc, char *argv[]) {
    std::string mystring = "the quick :brown fox ect";
    std::string mystring2 = "the quick bro*wn fox ect";
    std::string mystring3 = "the quick brown fox ect";

    printpair(split_by_delimiter(mystring, ':'));
    printpair(split_by_delimiter(mystring2, '*'));
    printpair(split_by_delimiter(mystring3, ' '));
    printpair(split_by_delimiter(mystring3, '$'));
    printpair(split_by_delimiter("", ' '));
    return 0;
}
