#include "chessBoard.h"
#include <cassert>
#include <print>
#include "pieceMovements.hpp"

int main (int argc, char *argv[]) {
    char myargstring[256];
    auto counter {0uz};
    for (auto arr : std::span(argv, argc)) {
        if (counter == 0) {
            counter ++;
            continue;
        }

        for (auto c : std::string_view(arr)) {
            assert(counter < 256);
            myargstring[counter-1] = c;
            counter ++;
        }   
        assert(counter < 256);
        myargstring[counter-1] = ' ';
        counter ++;
    }
    myargstring[counter-1] = '\0';
    std::string_view fenArgument = argc > 1 ? std::string_view(myargstring) : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    chessBoard board{fenArgument};
    
    auto allMoves = chessMoves::makeAllMoves(board);

    std::println("{} moves in this position", allMoves.numitems());

    return 0;
}
