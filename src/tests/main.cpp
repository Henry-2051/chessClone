#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pieceMovements.hpp"
#include "testFastStack.h"

int factorial(int number) {
    return number <= 1 ? number : factorial(number - 1) * number;
}


TEST_CASE("Basic test of movegen function") {
    auto myBoard = chessBoard();
    CHECK(chessMoves::makeAllMoves(myBoard).numitems() == 20);
};

