#include "chessBoard.h" 
#include "helpers.hpp"
#include "pieceMovements.hpp"
#include "stackStack.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <functional>
#include <future>
#include <iterator>
#include <map>
#include <mutex>
#include <numeric>
#include <optional>
#include <print>
#include <stdexcept>
#include <stop_token>
#include <string_view>
#include <sys/types.h>
#include <thread>
#include <utility>
#include "threaddedBuffer.h"


// negamax search

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

// the method works by bitwise anding the uint64 score bitboard with the piece bitboard and calling std::popcount 
// to determine the number of 1 bits in the result, then multiplying by the weighting 

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

int slowEval(const chessBoard& board, bool endgame=false) {
    return materialScore(board) + positionalScoreSquarewise(board, endgame);
};

int pieceWiseEval(const chessBoard& board, bool endgame=false) {
    return materialScore(board) + positionalScorePiecewise(board, endgame);
};

using EvalFunction = int (*) (const chessBoard&, bool);

template<EvalFunction eval>
inline int negaMax(int depth, chessBoard board) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    if (depth == 0)
        return isWhiteTurn ? eval(board, false) : -eval(board, false);

    int max = -100000000;
    auto allMoves = chessMoves::makeAllMoves(board);
    for (auto mv : allMoves) {
        int score;

        // if(unmakeMove) {
            score = -negaMax<eval>(depth - 1, board.applyMoveImpure(mv));
            board.applyMoveImpure(mv);
        // } else {
        //     score = -negaMax<unmakeMove>(depth - 1, board.applyMovePure(mv));
        // }

        if (score > max)
            max = score;
    }

    return max;
}

struct searchAnswer {
    int evalScore;
    pieceMovement move;
    bool terminate = false; // used to terminate up the call stack

    searchAnswer& negate() {
        evalScore = -evalScore;
        return *this;
    }
};

struct searchState {
    int alpha;
    int beta;
    std::optional<pieceMovement> lastMove;

    inline searchState reflectPure() const {
        searchState st {*this};

        st.alpha = -beta;
        st.beta = -alpha;

        return st;
    }
};

struct searchTelemetry {
    size_t numEvals{0};
    size_t numMovegens {0};
    size_t nodes {0};
};

inline int scoreMove(const pieceMovement& mv, const chessBoard& board, const chessMoves::movegenEngineData& data, searchState st) {
    // weighting parameters
    
    // the idea behind this is we want to try all the captures that win us material first then all the captures
    // of equal material weight and then all the captures that lose material
    //
    // this plays out captue chains woah
    const int lastMoveCaptureRating{1000};

    // weighting parameters



    bool isWhiteTurn = board_state::WhiteTurn & board.m_board_state;
    uint64_t enemies = !isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t undefendedEnemyPieces = enemies & (~data.enemyAttacksMushed);
    int runningTotal {0};

    std::optional<uint64_t> squareMovedToLast {};
    if (st.lastMove.has_value()) {
        squareMovedToLast = st.lastMove->movement & board.slowPieceToBitboardConst(!isWhiteTurn, st.lastMove->movement1BlackBB);
    }

    // condition: detect a (non en passant) capture, 
    // white captures black
    if (mv.movement1WhiteBB != PieceType::NotAPiece && mv.movement2BlackBB != PieceType::NotAPiece) {

        if (squareMovedToLast.has_value() && (*squareMovedToLast & mv.secondMovement))
            runningTotal += lastMoveCaptureRating;

        int valueOfCapturedPiece {pMaps::pieceScoreArray[mv.movement2BlackBB].score};
        if (mv.secondMovement & undefendedEnemyPieces) {
            // scoreOnPieceMaterial has a static assert that verifies the indecies are correct
            runningTotal += valueOfCapturedPiece;
        } else {
            int valueOfOurPiece {pMaps::pieceScoreArray[mv.movement1WhiteBB].score};
            runningTotal +=  valueOfCapturedPiece - valueOfOurPiece;
        }
    } else if (mv.movement1BlackBB !=PieceType::NotAPiece && mv.movement2WhiteBB != PieceType::NotAPiece) {

        if (squareMovedToLast.has_value() && (*squareMovedToLast & mv.secondMovement))
            runningTotal += lastMoveCaptureRating;

        int valueOfCapturedPiece {pMaps::pieceScoreArray[mv.movement2WhiteBB].score};
        if (mv.secondMovement & undefendedEnemyPieces){
            runningTotal += valueOfCapturedPiece;
        } else {
            // we want to incentivise capturing a piece even if it loses material in the short term
            // for properly evaluating capture sequences 
            int valueOfOurPiece {pMaps::pieceScoreArray[mv.movement1BlackBB].score};
            runningTotal += valueOfCapturedPiece - valueOfOurPiece;
        }
    }

    // // en passant capture detection
    // if (  ((isWhiteTurn ? mv.movement1WhiteBB : mv.movement1BlackBB) == PieceType::Pawn) 
    //         && mv.secondMovement 
    //         && !(mv.movement & mv.secondMovement)) 
    // {
    //
    // }

    return runningTotal;
}

// returns a permutation array of sorted moves from best to worst
// the iterator is a sentinal value 
inline FastStack<size_t, 218> orderMoves(const stackStack218& unorderedMoves, const chessBoard& board, 
                                         const chessMoves::movegenEngineData& data, searchState st) {
    std::array<int, 218> moveScores {};
    FastStack<size_t, 218> permutations {};
    {
        size_t scoreIdx {0};
        for (const auto& mv : unorderedMoves) {
            moveScores[scoreIdx] = scoreMove(mv, board, data, st);
            permutations.pushVal(scoreIdx);
            scoreIdx ++;
        }
    }

    std::sort(permutations.begin(), permutations.end(), [&](size_t idx1, size_t idx2) {
            // if this is true the left element goes first
            return moveScores[idx1] < moveScores[idx2];
            });

    return permutations;
}


namespace AlphaBeta {
    template <EvalFunction eval>
    searchAnswer alphaBeta(searchState st, chessBoard board, int depthleft, searchTelemetry* tele=nullptr, std::optional<std::stop_token> stop_token = {}) {
        bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
        // bool endgame = false;
        searchAnswer bestMove {-100000000};
        
        auto [allMoves, moveGenData] = chessMoves::makeAllMovesWithDataReturn(board);

        if (depthleft == 0 || allMoves.numitems() == 0) {
            if (tele != nullptr)
                tele->numEvals ++;
            return isWhiteTurn ? searchAnswer{eval(board, false)} : searchAnswer{-eval(board, false)}; 
        }

        // check at depth = 2, 
        if (depthleft == 2 && stop_token.has_value()) {
            if (stop_token->stop_requested()) {
                bestMove.terminate = true;
                return bestMove;
            }
        }

        FastStack<size_t, 218> sortedPerms {orderMoves(allMoves, board, moveGenData, st)};

        for (size_t idx : sortedPerms) {
        // for (const auto& mv : allMoves) {
            const pieceMovement& mv = allMoves[idx];
            st.lastMove = mv;
            chessBoard newBoard = board.applyMovePure(mv);

            auto searchReturn = alphaBeta<eval>(st.reflectPure(), newBoard, depthleft-1, tele, stop_token); 

            if (searchReturn.terminate) {
                return searchAnswer{-100000000, {}, true};
            }

            int score = -searchReturn.evalScore;

            if (score > bestMove.evalScore) {
                bestMove.evalScore = score;
                bestMove.move = mv;
            }

            if (score >= st.beta) {
                break;
            }

            st.alpha = std::max(st.alpha, score);
        }

        // std::println("");

        return bestMove;
    }

    template <EvalFunction eval>
    searchAnswer alphaBetaUnordered(searchState st, chessBoard board, int depthleft, searchTelemetry* tele = nullptr) {
        bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
        // bool endgame = false;
        searchAnswer bestMove {-100000000};
        // auto allMoves = chessMoves::makeAllMoves(board);
        
        auto [allMoves, moveGenData] = chessMoves::makeAllMovesWithDataReturn(board);

        if (depthleft == 0 || allMoves.numitems() == 0)
        {
            if (tele != nullptr)
                tele->numEvals ++;
            return isWhiteTurn ? searchAnswer{eval(board, false)} : searchAnswer{-eval(board, false)}; 
        }

        // FastStack<size_t, 218> sortedPerms {orderMoves(allMoves, board, moveGenData, st)};
        // std::println("{}",sortedMovePermutation[0]);

        // for (size_t idx : sortedPerms) {
        //     std::print("{}, ",idx);
        // }
        // std::println("");

        // for (size_t idx : sortedPerms) {
        for (const auto& mv : allMoves) {
            // const pieceMovement& mv = allMoves[idx];
            st.lastMove = mv;
            chessBoard newBoard = board.applyMovePure(mv);

            // reflectPure just does alpha = -beta and beta = -alpha 
            // we're doing this so that as the complexity of the search grows and we need to pass more 
            // state / information down the call stack our number of function parameters doesnt become 
            // unmanagable. 
            // for instance at the moment we pass down the last move so that move ordering 
            // can prioritise playing out capture chains.
            // later we might want to pass down principle variation and killer moves
            
            int score = -alphaBeta<eval>(st.reflectPure(), newBoard, depthleft-1, tele).evalScore;

            if (score > bestMove.evalScore) {
                bestMove.evalScore = score;
                bestMove.move = mv;
            }

            if (score >= st.beta) {
                break;
            }

            st.alpha = std::max(st.alpha, score);
        }

        // std::println("");

        return bestMove;
    }
}

template <EvalFunction eval>
inline searchAnswer alphaBeta(int depth, chessBoard board, searchTelemetry* tele= nullptr, std::optional<std::stop_token> stop_token = {}) {
    searchState st {-1000000000, 1000000000};
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    return isWhiteTurn ? AlphaBeta::alphaBeta<eval>(st, board, depth, tele, stop_token) : AlphaBeta::alphaBeta<eval>(st, board, depth, tele, stop_token).negate();
}

template <EvalFunction eval>
inline searchAnswer alphaBetaUnordered(int depth, chessBoard board, searchTelemetry* tele= nullptr) {
    searchState st {-1000000000, 1000000000};
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    return isWhiteTurn ? AlphaBeta::alphaBetaUnordered<eval>(st, board, depth, tele) : AlphaBeta::alphaBetaUnordered<eval>(st, board, depth, tele).negate();
}

struct threaddedSearchAnswer {
    std::mutex mu;
    std::optional<searchAnswer> answer;
};

struct argumentValue {
    std::optional<std::string> fen {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};
    std::optional<std::string> moves {};
};

inline
argumentValue argumentParser(int argc, char* argv[]) {
    argumentValue gs;
    // handle --uci 
    int lastLoadedParameter {argc};
    for (int i {argc}; i-- > 1;) {
        std::string uciString {"--uci"};
        if (std::strcmp(argv[i], uciString.c_str()) == 0) {
            // gs.uciMode = true;
            // Engine only handles uci
            lastLoadedParameter = i;
        }
    
        std::string fenString {"--fen"};
        if (std::strcmp(argv[i], fenString.c_str()) == 0) {
            gs.fen = "";
            for (int j {i+1}; j < lastLoadedParameter; j++) {
                if (!gs.fen.has_value())
                    gs.fen = argv[j];
                else 
                    *gs.fen += (std::string(" ") + argv[j]);
            }

            lastLoadedParameter = i;
        }
        std::string movesString {"--moves"};
        std::string moves {""};
        if (std::strcmp(argv[i], movesString.c_str()) == 0) {
            for (int j {i+1}; j < lastLoadedParameter; j++) {
                if (!gs.moves.has_value())
                    gs.moves = argv[j];
                else 
                    *gs.moves += (std::string(" ") + argv[j]);
            }

            lastLoadedParameter = i;
        }

    }
    return gs;
}

class chessEngine {
    bool m_isthinking {false};
    threaddedSearchAnswer m_sharedAnswer;
    std::optional<searchAnswer> m_answer {std::nullopt};
    std::jthread m_searchThread;

    public:
    chessBoard m_chessBoard;
    interfacePrinterState* m_printerState{nullptr};


    static void iterativeSearch(std::stop_token st, chessBoard board, threaddedSearchAnswer& sharedAnswer, interfacePrinterState* printerState = nullptr) {
        int depth {2};
        
        while(!st.stop_requested()) {
            // std::println("Executing to depth {}", depth);
            searchTelemetry telemetry {};
            auto start = std::chrono::steady_clock::now();
            auto ans = alphaBeta<pieceWiseEval>(depth, board, &telemetry, st);
            auto stop = std::chrono::steady_clock::now();
            if (!ans.terminate) {
                std::optional<pieceMovement> lastMove {std::nullopt};
                {
                    //another thread could be trying to write to this, causing a race condition
                    std::lock_guard<std::mutex > lock{sharedAnswer.mu};
                    lastMove = sharedAnswer.answer.has_value() ? std::optional<pieceMovement>(sharedAnswer.answer->move) : std::nullopt;
                    sharedAnswer.answer = ans;
                }

                if((lastMove.has_value() && !(lastMove->compareForSelection(ans.move))) || !lastMove.has_value()) {
                    auto searchTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                    std::string info = std::format("info currmove {} depth {} searchtime_ms {} numEvals {}", board.uciStringMove(ans.move), depth, searchTimeMs, telemetry.numEvals);

                    {
                    std::lock_guard<std::mutex> lock {printerState->stdoutBuffer.bufferMutex};
                    printerState->stdoutBuffer.buffer.push_back(std::move(info));
                    }

                    printerState->flushBuffer.notify_one();
                }
            }
            depth++;
        }
    }

    // chessEngine() = default;

    // replaces the internal chess board with one of the supplied position and moves, if no arguments are provided uses the default start position
    // note will not return false if given an invalid fenString
    // you cannont return a bool from a constructor without passing an extra reference
    bool 
    loadPosition(std::optional<std::string_view> fenString = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", std::optional<std::string_view> moves = {}) 
    {
        // TODO : add logging 
        // if (fenString.has_value())
        //     std::println("loading fen : {}", *fenString);
        // if (moves.has_value())
        //     std::println("loading moves : {}", *moves);
        //
        // if there is a value then load it 
        // otherwise we dont change the board state
        if (fenString == "startpos" || fenString == "") {
            m_chessBoard = chessBoard();
        } else {
            if (fenString.has_value()) {
                m_chessBoard = chessBoard(*fenString);
            }
        }

        bool sucess = moves.has_value() ? chessMoves::makeMovesFromUciSequence(m_chessBoard, *moves) : true;

        return sucess;
    };

    // starts searching in a seperate thread
    bool startSearch() {
        if (m_isthinking)
            return false;

        m_searchThread = std::jthread(chessEngine::iterativeSearch,  m_chessBoard, std::ref(m_sharedAnswer), m_printerState);
        m_isthinking = true;
        bool sucess = true;
        return sucess;
    };

    // updates the internal search answer state in a thread safe manner and returns the new state
    // if the engine isnt thinking then it just returns gets the most recent answer
    std::optional<searchAnswer> getAnswer() {
        if (m_isthinking) {
            m_sharedAnswer.mu.lock();
            m_answer = m_sharedAnswer.answer;
            m_sharedAnswer.mu.unlock();
        }

        return m_answer;
    };

    // stops all searching and updates the internal answer with the final threadded answer incase a new depth has been
    // completed since getAnswer was called
    bool stopSearching() {
        m_searchThread.request_stop();
        m_searchThread.join();
        m_isthinking = false;
        m_answer = m_sharedAnswer.answer;
        return true;
    };
};
