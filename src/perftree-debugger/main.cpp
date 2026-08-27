#include <cassert>
#include <string>
#include <string_view>
#include "perftreeRun.h"

void perftree(size_t perftnumber=1, std::string_view fenArgument="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", std::string_view movesToMake = "") {
    chessBoard board {fenArgument};

    if(movesToMake != "" && !chessMoves::makeMovesFromUciSequence(board, movesToMake))
        return;

    size_t numMoves;
    perftreeRun<false>(perftnumber, board, numMoves);
    
    // Timer<Timers::SlidingAttackRook>::printAverageTimeNanoseconds();
    //
    // Timer<Timers::SlidingAttackRook>::printTotalTimeMilliseconds();
}

int main (int argc, char *argv[]) {

    // perftreeRun<false>(3, board1);
    // perftree();
    switch(argc) {
        case (1) : perftree(); break;
        case (2) : perftree(std::stoul(argv[1])); break;
        case (3) : perftree(std::stoul(argv[1]), argv[2]); break;
        case (4) : perftree(std::stoul(argv[1]), argv[2], argv[3]); break;
    }

    return 0;
}
