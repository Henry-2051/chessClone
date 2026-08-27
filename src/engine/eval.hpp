
#include "chessBoard.h"
#include <cstdint>
#pragma once

struct positionalBucket{
    uint64_t bitboard;
    int score;
};

struct pieceScores {
    PieceType pieceType;
    int score;
};

namespace pMaps {
// we can make the eval function for a quiet position (with no available tactics, eg hanging pieces or forcing sequences) much
// stronger by giving pieces on better squares higher scores

// this is bascially a bucketing approach where we collect the bitmap of peices on certain valued squares then run a popcount
// the placement bitmaps have been checked against an array implementation. 
//
// this appreach gains us an about 5x speedup on eval which is very high impact

static constexpr std::array<positionalBucket, 9> pawnPlacementScores {
    positionalBucket{65280, 50},
    positionalBucket{1572864, 30},
    positionalBucket{402653184, 25},
    positionalBucket{103081574400, 20},
    positionalBucket{28710448241246208, 10},
    positionalBucket{36452112267214848, 5},
    positionalBucket{72567767433216, -5},
    positionalBucket{39582418599936, -10},
    positionalBucket{6755399441055744, -20}
};

static constexpr std::array<positionalBucket, 8> knightPlacementScores {
    positionalBucket{0x1818000000,   20},
    positionalBucket{0x182424180000, 15},
    positionalBucket{0x240000240000, 10},
    positionalBucket{0x420042000000, 5},
    positionalBucket{0x42000000004200, -20},
    positionalBucket{0x3c0081818181003c, -30},
    positionalBucket{0x4281000000008142, -40},
    positionalBucket{0x8100000000000081, -50}
};

static constexpr std::array<positionalBucket, 4> bishopPlacemntScores {
    positionalBucket{0x7e3c18180000, 10},
    positionalBucket{0x42000066240000, 5},
    positionalBucket{0x7e8181818181817e, -10},
    positionalBucket{0x8100000000000081, -20},
};

static constexpr std::array<positionalBucket, 3> rookPlacementScores {
    positionalBucket{0x7e00, 10},
    positionalBucket{0x1800000000008100, 5},
    positionalBucket{0x81818181810000, -5}
};  

static constexpr inline std::array<positionalBucket, 4> queenPlacementScores {
    positionalBucket{0x43e3c3c3c0000, 5},
    positionalBucket{0x1800008081000018, -5},
    positionalBucket{0x6681810000818166, -10},
    positionalBucket{0x8100000000000081, -20}
};

static constexpr std::array<positionalBucket, 8> kingMiddlegameScores {
    positionalBucket{0x18181818, -50},
    positionalBucket{0x1866666666, -40},
    positionalBucket{0x6681818181, -30},
    positionalBucket{0x7e8100000000, -20},
    positionalBucket{0x810000000000, -10},
    positionalBucket{0x2400000000000000, 10},
    positionalBucket{0x81c3000000000000, 20},
    positionalBucket{0x4200000000000000, 30},
};

static constexpr std::array<positionalBucket, 8> kingEndgameScores {
    positionalBucket{0x1818000000, 40},
    positionalBucket{0x182424180000, 30},
    positionalBucket{0x240000240000, 20},
    positionalBucket{0x424242422400, -10},
    positionalBucket{0x4218, -20},
    positionalBucket{0x7ec3818181818124, -30},
    positionalBucket{0x42, -40},
    positionalBucket{0x8100000000000081, -50}
};

template<std::array<pieceScores, 6> scoreArray>
constexpr bool n_validatePieceScoreArray() {
    constexpr PieceType pieces[6] {
        PieceType::Pawn, PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen, PieceType::King
    };

    for (auto pt : pieces) {
        if(scoreArray[pt].pieceType != pt)
            return false;
    }
    return true;
}

static constexpr std::array<pieceScores, 6> pieceScoreArray{
    pieceScores{PieceType::Pawn, 100},
    pieceScores{PieceType::Rook, 500},
    pieceScores{PieceType::Knight, 320},
    pieceScores{PieceType::Bishop, 330},
    pieceScores{PieceType::Queen, 900},
    pieceScores{PieceType::King, 20000},
};

// experimenting with compile time functions 
static_assert(n_validatePieceScoreArray<pieceScoreArray>(), "error piece score array is invalid");
}
namespace placementArrays {
constexpr size_t arrayLen = 64 * 7;
constexpr int8_t piecePlacement[arrayLen] {
// how to turn this into bucket bitboards 
//
// basically what you want to do is create a function with a templated return of an array of uint64_t with a template size 
// then you want a second function that finds out how many different squares values are in an array of square values, this 
// needs to be constexpr
//
// next you want to write your second constexpr function that takes an array of square values, in the first pass it creates
// an array of the different bucket values eg {-20, -5, 5, 10, 20, 25, 30, 50} then on the second pass for each non zero  
// square it finds the index in the first array of the squres then does a | operation with 1ULL << squareValueIndex to flip 
// the bit buckets corresponding bit to 1 from zero. do this over all 64 squares and we create a compiletime bitbucket generator
// and no more hardcoded bitboards
//
// Pawn
//
// we like having pawns infront of the king 
// and having pawns that are about to promote is also really good
// infact having past pawns is really good as well but we cant encode that heuristic in this scheme 
 0,  0,  0,  0,  0,  0,  0,  0,
50, 50, 50, 50, 50, 50, 50, 50,
10, 10, 20, 30, 30, 20, 10, 10,
 5,  5, 10, 25, 25, 10,  5,  5,
 0,  0,  0, 20, 20,  0,  0,  0,
 5, -5,-10,  0,  0,-10, -5,  5,
 5, 10, 10,-20,-20, 10, 10,  5,
 0,  0,  0,  0,  0,  0,  0,  0,

// Rook
//
// vaguely we want to centeralise the rook, we also want it looking down either the e or d files
// its also good if we can get it to the 7th rank if we're white
  0,  0,  0,  0,  0,  0,  0,  0,
  5, 10, 10, 10, 10, 10, 10,  5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
  0,  0,  0,  5,  5,  0,  0,  0,

// Knight
//
// we really want our knights in the center of the board where they see a lot of squares 
// also we are sad if they are in the corners or edges of the board
-50,-40,-30,-30,-30,-30,-40,-50,
-40,-20,  0,  0,  0,  0,-20,-40,
-30,  0, 10, 15, 15, 10,  0,-30,
-30,  5, 15, 20, 20, 15,  5,-30,
-30,  0, 15, 20, 20, 15,  0,-30,
-30,  5, 10, 15, 15, 10,  5,-30,
-40,-20,  0,  5,  5,  0,-20,-40,
-50,-40,-30,-30,-30,-30,-40,-50,

// Bishop
//
// we generally want our bishops on the long diagonal 
-20,-10,-10,-10,-10,-10,-10,-20,
-10,  0,  0,  0,  0,  0,  0,-10,
-10,  0,  5, 10, 10,  5,  0,-10,
-10,  5,  5, 10, 10,  5,  5,-10,
-10,  0, 10, 10, 10, 10,  0,-10,
-10, 10, 10, 10, 10, 10, 10,-10,
-10,  5,  0,  0,  0,  0,  5,-10,
-20,-10,-10,-10,-10,-10,-10,-20,

// Queen
//
-20,-10,-10, -5, -5,-10,-10,-20,
-10,  0,  0,  0,  0,  0,  0,-10,
-10,  0,  5,  5,  5,  5,  0,-10,
 -5,  0,  5,  5,  5,  5,  0, -5,
  0,  0,  5,  5,  5,  5,  0, -5,
-10,  5,  5,  5,  5,  5,  0,-10,
-10,  0,  5,  0,  0,  0,  0,-10,
-20,-10,-10, -5, -5,-10,-10,-20,

// King middlegame
//
// in the middle game we want to incentivise keeping the king on our side of the board behind our pieces 
// and we want to heavilly discourage walking the king out into the center of the board, this is not good
-30,-40,-40,-50,-50,-40,-40,-30,
-30,-40,-40,-50,-50,-40,-40,-30,
-30,-40,-40,-50,-50,-40,-40,-30,
-30,-40,-40,-50,-50,-40,-40,-30,
-20,-30,-30,-40,-40,-30,-30,-20,
-10,-20,-20,-20,-20,-20,-20,-10,
 20, 20,  0,  0,  0,  0, 20, 20,
 20, 30, 10,  0,  0, 10, 30, 20,

// King endgame
//
// we want to do almost the opposite, we want the king in the center controlling as many squares as possible, we 
// would prefer if the king isnt at the edges of the board here
-50,-40,-30,-20,-20,-30,-40,-50,
-30,-20,-10,  0,  0,-10,-20,-30,
-30,-10, 20, 30, 30, 20,-10,-30,
-30,-10, 30, 40, 40, 30,-10,-30,
-30,-10, 30, 40, 40, 30,-10,-30,
-30,-10, 20, 30, 30, 20,-10,-30,
-30,-30,  0,  0,  0,  0,-30,-30,
-50,-30,-30,-30,-30,-30,-30,-50,
};
};

inline 
int positionalScoreSquarewise(const chessBoard& board, bool endgame) {
    int score {0};
    for (int idx {64}; (idx --) != 0; ) {
        PieceType ptWhite = board.figureOutTypeOfPieceOnSquare(1ULL << idx, true);
        PieceType ptBlack = board.figureOutTypeOfPieceOnSquare(1ULL << idx, false);
        int idxCpy {static_cast<int>(idx)};

        if (ptWhite != PieceType::NotAPiece ) {

            if (ptWhite == PieceType::King && endgame)
                idxCpy += 64;
            idxCpy += ptWhite* 64;

            int specificScore {static_cast<int>(placementArrays::piecePlacement[idxCpy])};
            score += specificScore;
            // std::println("white {}({}) at index {} with array index {}, and specific score {}", getPieceTypeString(ptWhite), std::to_underlying(ptWhite), idx, idxCpy, specificScore);

        } else if (ptBlack != PieceType::NotAPiece) {
            // reflect along the horizontal line
            int rank {idx / 8};
            int file {idx % 8};
            rank = 7-rank;
            idxCpy = rank * 8 + file;

            if (ptBlack == PieceType::King && endgame)
                idxCpy += 64;
            idxCpy += ptBlack * 64;
            int specificScore {-static_cast<int>(placementArrays::piecePlacement[idxCpy])};

            score += specificScore;

            // int idx2 {idxCpy % 64};
            // int bbindex {idxCpy  /64};
            // rank =idx2 / 8;
            // file =idx2 % 8;
            // std::println("black {}({}) at index {} with array index {}[{}, {}] board [{}], and specificScore {}", getPieceTypeString(ptBlack), std::to_underlying(ptBlack), idx, idxCpy,rank, file, bbindex, specificScore);
        } else {
            continue;
        }

        assert(idxCpy < arrayLen);
    }
    return score;
}

// swapping out the for loop with this causes the program to run roughly 5 times faster, on brute force minimax we go to depth 5 in 500ms
template <PieceType pt, size_t N>
int scoreOnPiecePositionPiecewise(std::array<positionalBucket, N> scoreBitboardBuckets, const chessBoard& board) {
    int score {0};
    // std::println("inside score on piece position piecewise");
    // std::println("operating on piece : {}", getPieceTypeString(pt));
    for (const positionalBucket& bucket : scoreBitboardBuckets) {
        score += std::popcount(bucket.bitboard & board.pieceToBitboardConst<pt>(true)) * bucket.score;

        uint64_t flippedBitboard = std::byteswap(bucket.bitboard);

        // std::println("worth {}", -bucket.score);
        // helpers::printBitboard(flippedBitboard);
        // std::println();

        score += std::popcount(flippedBitboard & board.pieceToBitboardConst<pt>(false)) * (-bucket.score);
    }
    return score;
}

inline int positionalScorePiecewise(const chessBoard& board, bool endgame) {
    // we would like to swap out the hard coded arrays for const eval functions which rely on the readable piece placement arrays, thus eliminating the duplicated hard coded positional information
    // indeed doing this would allow for further optimising of the placement scoring using automated methods like evolutionary algorithms, although we will probabably implement nnue before this, making
    // this method obsolete
    return  scoreOnPiecePositionPiecewise<Pawn>(pMaps::pawnPlacementScores, board)
          + scoreOnPiecePositionPiecewise<Knight>(pMaps::knightPlacementScores, board)
          + scoreOnPiecePositionPiecewise<Bishop>(pMaps::bishopPlacemntScores, board)
          + scoreOnPiecePositionPiecewise<Rook>(pMaps::rookPlacementScores, board)
          + scoreOnPiecePositionPiecewise<Queen>(pMaps::queenPlacementScores, board)
          + (endgame ? scoreOnPiecePositionPiecewise<King>(pMaps::kingEndgameScores, board) : scoreOnPiecePositionPiecewise<King>(pMaps::kingMiddlegameScores, board));
}

template <PieceType pt>
int scoreOnPieceMaterial(const chessBoard& board) {
    constexpr pieceScores ps {pMaps::pieceScoreArray[std::to_underlying(pt)]};

    static_assert(ps.pieceType == pt, "indexing into an arry with an enum, if the enum ever changes it will break this function, the program should not compile in this case");

    int score = std::popcount(board.pieceToBitboardConst<pt>(true)) * ps.score; // number of white peices
    score -= std::popcount(board.pieceToBitboardConst<pt>(false)) * ps.score;   // num black
    return score;
}

inline int materialScore(const chessBoard& board) {
    return scoreOnPieceMaterial<Pawn>(board)   + 
           scoreOnPieceMaterial<Rook>(board)   + 
           scoreOnPieceMaterial<Knight>(board) + 
           scoreOnPieceMaterial<Bishop>(board) + 
           scoreOnPieceMaterial<Queen>(board)  + 
           scoreOnPieceMaterial<King>(board);
}

inline int slowEval(const chessBoard& board, bool endgame=false) {
    return materialScore(board) + positionalScoreSquarewise(board, endgame);
};

inline int pieceWiseEval(const chessBoard& board, bool endgame=false) {
    return materialScore(board) + positionalScorePiecewise(board, endgame);
};
