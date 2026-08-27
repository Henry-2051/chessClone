#include "transpositionTable.h"
#include "search.h"
#include "eval.hpp"
#include "engineSharedDatatypes.hpp"
#include "transpositionTable.h"
#include "chessBoard.h"
#include "pieceMovements.hpp"
#include "eval.hpp"
#include <algorithm>
#include <cstdint>
#include <print>


struct searchAnswerInternal {
    searchAnswer answer;
    uint8_t bestMoveIdx;
    bool hasBestMoveIdx{false};
};

struct ttEntryCopy {
    TT::tableEntry remoteEntry;
    uint64_t localZHash;
    bool remoteVerified {false};
};

inline int scoreCapture(const pieceMovement& mv, uint64_t uep) {
    // white captures black
    bool isWhiteCapture = mv.movement1WhiteBB != PieceType::NotAPiece && mv.movement2BlackBB != PieceType::NotAPiece;
    bool isBlackCapture = mv.movement1BlackBB != PieceType::NotAPiece && mv.movement2WhiteBB != PieceType::NotAPiece;

    if (isWhiteCapture) {
        // the score of capturing an undefended piece is equal to that pieces value
        int valueOfCapturedPiece {pMaps::pieceScoreArray[mv.movement2BlackBB].score};
        if (mv.secondMovement & uep) {
            return valueOfCapturedPiece;
        } else {
            // otherwise we score it as the value of the enemy piece minus the value of our piece
            int valueOfOurPiece {pMaps::pieceScoreArray[mv.movement1WhiteBB].score};
            return  valueOfCapturedPiece - valueOfOurPiece;
        }
    // black captures white
    } else if (isBlackCapture) {
        int valueOfCapturedPiece {pMaps::pieceScoreArray[mv.movement2WhiteBB].score};
        if (mv.secondMovement & uep){
            return valueOfCapturedPiece;
        } else {
            int valueOfOurPiece {pMaps::pieceScoreArray[mv.movement1BlackBB].score};
            return valueOfCapturedPiece - valueOfOurPiece;
        }
    }
    return 0;
}

inline int scorePromotion(const pieceMovement& mv, uint64_t uep) {
    bool whitePromotion = mv.movement1WhiteBB == PieceType::Pawn && mv.movement2WhiteBB != PieceType::NotAPiece;
    bool blackPromotion = mv.movement1BlackBB == PieceType::Pawn && mv.movement2BlackBB != PieceType::NotAPiece;

    if(whitePromotion) 
    {
        uint64_t promotionVal = pMaps::pieceScoreArray[mv.movement2WhiteBB].score - pMaps::pieceScoreArray[PieceType::Pawn].score;
        bool isWhiteCapture = mv.movement1WhiteBB != PieceType::NotAPiece && mv.movement2BlackBB != PieceType::NotAPiece;
        bool isUndefendedWhiteCapture = isWhiteCapture && (mv.movement2BlackBB & uep);

        if (isUndefendedWhiteCapture) {
            return promotionVal + pMaps::pieceScoreArray[mv.movement2BlackBB].score;
        } else if (isWhiteCapture) {
            return pMaps::pieceScoreArray[mv.movement2BlackBB].score - pMaps::pieceScoreArray[PieceType::Pawn].score;
        }

        return promotionVal;
    } 
    else if (blackPromotion) 
    {
        uint64_t promotionVal = pMaps::pieceScoreArray[mv.movement2BlackBB].score - pMaps::pieceScoreArray[PieceType::Pawn].score;
        bool isBlackCapture = mv.movement1BlackBB != PieceType::NotAPiece && mv.movement2WhiteBB != PieceType::NotAPiece;
        bool isUndefendedBlackCapture = isBlackCapture && (mv.movement2WhiteBB & uep);

        if (isUndefendedBlackCapture) {
            return promotionVal + pMaps::pieceScoreArray[mv.movement2WhiteBB].score;
        } else if (isBlackCapture) {
            return pMaps::pieceScoreArray[mv.movement2WhiteBB].score - pMaps::pieceScoreArray[PieceType::Pawn].score;
        }

        return promotionVal;
    }

    return 0;
}


inline int 
scoreMove(const pieceMovement& mv, const chessBoard& board, const chessMoves::movegenEngineData& data, searchState st) {
    // weighting parameters
    
    // the idea behind this is we want to try all the captures that win us material first, then non capturing moves and captures of equal material
    // and lastly captures that lose material
    //
    // this plays out captue chains woah
    //
    // TODO : promote pawn promotions by pawn promotion value
    const int lastMoveCaptureRating{1000};

    // weighting parameters

    bool isWhiteTurn = board_state::WhiteTurn & board.m_board_state;
    uint64_t enemies = !isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t undefendedEnemyPieces = enemies & (~data.enemyAttacksMushed);
    int runningTotal {0};

    // we want to prioritise capturing the last moved piece to play out capture chains
    
    if (st.lastMove.has_value()) {
        uint64_t squareMovedToLast = st.lastMove->movement & board.slowPieceToBitboardConst(!isWhiteTurn, st.lastMove->movement1BlackBB);
        if (squareMovedToLast & mv.secondMovement) 
            runningTotal += lastMoveCaptureRating;
    }

    // scorePromotion handles capture promotions
    {
        int promotionVal {scorePromotion(mv, undefendedEnemyPieces)};
        int captureVal   {  scoreCapture(mv, undefendedEnemyPieces)};

        runningTotal += (promotionVal != 0) ? promotionVal : captureVal;
    }

    // fun idea, what if we inflated the en passant capture probability so the engine always capures en passant 
    //
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
inline FastStack<size_t, 218> 
orderMoves(const stackStack218& unorderedMoves, const chessBoard& board, const chessMoves::movegenEngineData& data, 
        searchState st) 
{
    std::array<int, 218> moveScores {};
    FastStack<size_t, 218> permutations {};
    {
        size_t scoreIdx {0};
        for (const auto& mv : unorderedMoves) {
            moveScores[scoreIdx] = scoreMove(mv, board, data, st) ;
            permutations.pushVal(scoreIdx);
            scoreIdx ++;
        }
    }


    std::sort(permutations.begin(), permutations.end(), [&](size_t idx1, size_t idx2) {
            // if this is true the left element goes first
            return moveScores[idx1] > moveScores[idx2];
            });

    return permutations;
}

// returns a permutation array of sorted moves from best to worst
inline FastStack<size_t, 218> 
orderMoves(const stackStack218& unorderedMoves, const chessBoard& board, const chessMoves::movegenEngineData& data, 
        searchState st, const std::array<ttEntryCopy, 218>& ttEntries, transpositionTableAccess& tableAccess,
        TT::tableEntry* currentTTEntryVerified = nullptr) 
{
    const int previousBestMoveScore = 1000;
    std::array<int, 218> moveScores {};
    FastStack<size_t, 218> permutations {};
    {
        size_t scoreIdx {0};
        for (const auto& mv : unorderedMoves) {
            const TT::tableEntry& entry = ttEntries[scoreIdx].remoteEntry;
            bool isValid = ttEntries[scoreIdx].remoteVerified;

            moveScores[scoreIdx] = scoreMove(mv, board, data, st) + (isValid ? entry.score : 0);
            permutations.pushVal(scoreIdx);
            scoreIdx ++;
        }
    }

    // use the transposition table entry to increase the score of the previous best move
    if (currentTTEntryVerified != nullptr) {
        moveScores[currentTTEntryVerified->bestMoveIdx] += previousBestMoveScore;
    }

    std::sort(permutations.begin(), permutations.end(), [&](size_t idx1, size_t idx2) {
            // if this is true the left element goes first
            return moveScores[idx1] > moveScores[idx2];
            });

    return permutations;
}

enum class QuietReturnState {
    Normal,
    WhiteCheckmate,
    BlackCheckmate,
    Draw,
};

struct quiessenceSearchReturn {
    int score;
    SearchReturnState rState;
};

inline bool isQuietAfterMove(chessBoard board, const pieceMovement& mv, uint64_t enemyAttacksMushed) {

    bool isWhiteTurn = (board.m_board_state & board_state::WhiteTurn);
    PieceType movePieceType = isWhiteTurn ? mv.movement1WhiteBB : mv.movement1BlackBB;

    // if the piece that we're moving is already attacked then there will be a discovery
    if (mv.movement & board.slowPieceToBitboardConst(isWhiteTurn, movePieceType) & enemyAttacksMushed)
        return false;

    board.applyMoveImpure(mv);

    uint64_t friendlyPieces = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    uint64_t friendlyPiecesNoPawn = friendlyPieces & ~board.pieceToBitboardConst<PieceType::Pawn>(isWhiteTurn);
    
    return !(friendlyPiecesNoPawn & enemyAttacksMushed);
}

// play out the most promising tactical move until there are no more promising tactical moves
// since we are just playing out a capture chain we can discard every move other than the one 
// we choose
template<EvalFunction eval>
int
search::quiessenceSearch(chessBoard board, stackStack218& allMovesMemory, bool _madeNullMove) {
    chessMoves::movegenEngineData metadata = chessMoves::makeAllMovesWithDataReturn(board, allMovesMemory);
    stackStack218& allMoves = allMovesMemory;

    bool     isWhiteTurn = board_state::WhiteTurn & board.m_board_state;
    uint64_t enemies = !isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t undefendedEnemyPieces = enemies & (~metadata.enemyAttacksMushed);

    int      bestMoveScore {0};
    size_t bestMoveIdx {0};

    for (size_t idx {allMoves.numitems()}; idx -- > 0;) {
        // std::println("idx : {}", idx);

        // int captureScore {scoreCapture(allMoves[idx], undefendedEnemyPieces)};
        // int promotionScore {scorePromotion(allMoves[idx], undefendedEnemyPieces)};
        // int score = promotionScore != 0 ? promotionScore : captureScore;

        int score {scoreCapture(allMoves[idx], undefendedEnemyPieces)};

        // std::println("selection score for {} : {}", board.uciStringMove(allMoves[idx]), score);
        if (score !=0 && score >= bestMoveScore) {
            bestMoveScore = score;
            bestMoveIdx = idx;
        }
    }

    int staticEvalScore = isWhiteTurn ? eval(board, false) : -eval(board, false);
    if (bestMoveScore != 0 && bestMoveScore > 10) {
        // auto mvString = board.uciStringMove(allMoves[bestMoveIdx]);
        // std::println("playing {}", board.uciStringMove(allMoves[bestMoveIdx]));
        
        // we dont know whether the capture is good, we may choose to play a quiet move instead
        // this relies on the assumption that there exists a move which preserves all our material on the board
        // the idea of this is that its going to increase the strength of the engine more than it decrements it
        // futhermore we can exploit bitboards to write an optimised movegen that only considers captures
        auto res =  std::max(-quiessenceSearch<eval>(board.applyMovePure(allMoves[bestMoveIdx]), allMovesMemory, false), staticEvalScore);
        // std::println("score for playing {} : {}", mvString, res);
        return res;
    }
    // std::println("static eval score {}", staticEvalScore);

    return staticEvalScore;
};

template 
int search::quiessenceSearch<pieceWiseEval>(chessBoard board, stackStack218& allMovesMemory, bool _madeNullMove);

namespace AlphaBeta {

int negaMaxEval (const chessBoard& boardBefore, const pieceMovement& mv, uint64_t enemyAttacksMushed) {
    bool isWhiteTurn = boardBefore.m_board_state & board_state::WhiteTurn;

    // return (isWhiteTurn) ? pieceWiseEval(boardBefore.applyMovePure(mv)) : -pieceWiseEval(boardBefore.applyMovePure(mv));

    stackStack218 allMoveMem;
    if (!isQuietAfterMove(boardBefore, mv, enemyAttacksMushed)) 
        return -search::quiessenceSearch<pieceWiseEval>(boardBefore.applyMovePure(mv), allMoveMem);
    else 
        return (!isWhiteTurn) ? pieceWiseEval(boardBefore.applyMovePure(mv)) : -pieceWiseEval(boardBefore.applyMovePure(mv));
}

searchAnswer 
noMovesEvalulation(searchState st, const chessBoard& board, const chessMoves::movegenEngineData& mgenData, int depthLeft) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t ourKing = board.pieceToBitboardConst<PieceType::King>(isWhiteTurn);
    std::println("nomoves");
    // we are checkmated
    if (mgenData.enemyAttacksMushed & ourKing) {
        // we would rather be mated in 3 plys than 1 ply
        // (st.rootSearchDepth - depthLeft) is how far we are from the root
        //
        int distanceFromRoot = st.rootSearchDepth - depthLeft;
        return searchAnswer{(-pMaps::pieceScoreArray[PieceType::King].score) + distanceFromRoot, {}, SearchReturnState::CheckmateOrDraw};
    } else {
        return searchAnswer{0, {}, SearchReturnState::CheckmateOrDraw};
    }

}

searchAnswerInternal
alphaBetaDepth1(searchState st, const chessBoard& board, const stackStack218& movesForBoard, const chessMoves::movegenEngineData& mgenData, searchTelemetry* tele = nullptr) {
    if (movesForBoard.numitems() == 0) {
        return searchAnswerInternal{noMovesEvalulation(st, board, mgenData, 1), 0, false};
    }

    searchAnswerInternal bestMove {searchAnswer{-INF, {}, SearchReturnState::Normal}};

    for (const auto& mv : movesForBoard) {
        
        int score = negaMaxEval(board, mv, mgenData.enemyAttacksMushed);
        tele->nodes ++;
        
        if (score > st.alpha) {
            bestMove.answer = searchAnswer{score, mv, SearchReturnState::Normal};
        }

        if (score >= st.beta) {
            bestMove.answer.returnState = SearchReturnState::BetaCutoff;
            break;
        }

        if (score > st.alpha) {
            st.alpha = score;
        }
    }

    if (bestMove.answer.returnState != SearchReturnState::BetaCutoff) {
        auto currentDepth = st.rootSearchDepth - 1;
        auto &pvNode = tele->principleVariation[currentDepth];
        auto &pvBoard= tele->boardStates[currentDepth];
        pvNode = bestMove.answer;
        pvBoard = board.applyMovePure(pvNode.bestMove);
    }

    return bestMove;
}

    template <bool hasTranspositionTable>
    searchAnswerInternal 
    alphaBetaInner(searchState st, chessBoard board, int depthleft, transpositionTableAccess& tableAccess, 
            const stackStack218& allMoves, const chessMoves::movegenEngineData& mgendata, 
            searchTelemetry* tele=nullptr, std::optional<std::stop_token> stop_token = {}, TT::tableEntry* currentTTEntryVerified = nullptr) 
    { 
        if (allMoves.numitems() == 0) {
            return searchAnswerInternal{noMovesEvalulation(st, board, mgendata, depthleft), 0, false};
        }
        if (stop_token.has_value() && stop_token->stop_requested()) {
            return searchAnswerInternal{searchAnswer{-INF, {}, SearchReturnState::SearchTerminated}};
        }

        std::array<ttEntryCopy, 218> ttEntries{};

        // if (hasTranspositionTable && false) {
        //     for (auto idx {allMoves.numitems()}; idx -- > 0;) {
        //         const auto& mv {allMoves[idx]};
        //         chessBoard newBoard = board.applyMovePure(mv);
        //
        //         ttEntries[idx].localZHash = tableAccess.interface->hashPosition(newBoard);
        //         ttEntries[idx].remoteEntry = tableAccess.interface->positionFromZHash(ttEntries[idx].localZHash, tableAccess.table);
        //
        //         TT::tableEntry& entry = ttEntries[idx].remoteEntry;
        //         ttEntries[idx].remoteVerified = entry.zHash == ttEntries[idx].localZHash;
        //     }
        // }

        // we ideally order the moves using our transposition table
        // FastStack<size_t, 218> sortedPerms {
        //     hasTranspositionTable ? orderMoves(allMoves, board, mgendata, st, ttEntries, tableAccess, currentTTEntryVerified) 
        //                           : orderMoves(allMoves, board, mgendata, st)
        // };
        
        FastStack<size_t, 218> sortedPerms {orderMoves(allMoves, board, mgendata, st)};
        searchAnswerInternal bestMove {searchAnswer{-INF, {}, SearchReturnState::Normal}};

        for (size_t idx : sortedPerms) {
            const pieceMovement& mv = allMoves[idx];
            st.lastMove = mv;
            chessBoard newBoard = board.applyMovePure(mv);

            searchAnswerInternal searchReturn;
            // the mission of this code block is to populate searchReturn and add an entry to the transposition table
            {
                // if (hasTranspositionTable && depthleft > 2 && false) {
                //     stackStack218 movesInner;
                //     chessMoves::movegenEngineData data = chessMoves::makeAllMovesWithDataReturn(newBoard, movesInner);
                //     std::pair<const stackStack218&, const chessMoves::movegenEngineData&> mGenPair {movesInner, data};
                //
                //     // whether the table entry refers to the same position, since we use modular arithmatic to constrain the size of 
                //     // the hash table we must verify the hashes, in case of a true collision we also check the number of moves
                //
                //     TT::tableEntry& entry = ttEntries[idx].remoteEntry;
                //     bool ttEntryVerified = ttEntries[idx].remoteIsValid;
                //     ttEntryVerified &= movesInner.numitems() == entry.numMoves;
                //
                //     if (ttEntryVerified && entry.depthSearched == depthleft-1) {
                //
                //         searchReturn = {{-entry.score, movesInner[entry.bestMoveIdx]}, entry.bestMoveIdx};
                //     }
                //     else {
                //         searchReturn = alphaBetaInner<eval, hasTranspositionTable>(st.reflectPure(), newBoard, depthleft-1, tableAccess, mGenPair, tele, stop_token, ttEntryVerified ? &entry : nullptr); 
                //
                //         TT::tableEntry candiate {ttEntries[idx].positionHash, searchReturn.answer.evalScore, newBoard.numPlys, depthleft-1, searchReturn.bestMoveIdx, static_cast<uint8_t>(movesInner.numitems())};
                //
                //         tableAccess.interface->positionFromZHash(ttEntries[idx].positionHash, tableAccess.table) = TT::tableSelectionFunction(entry, candiate, st.rootNumPlys);
                //     }
                // } else 
                {
                    stackStack218 moves;
                    chessMoves::movegenEngineData data = chessMoves::makeAllMovesWithDataReturn(newBoard, moves);
                    if (moves.numitems() == 0)
                        searchReturn.answer = noMovesEvalulation(st.reflectPure(), newBoard, data, depthleft - 1);
                    else if (depthleft <= 2)
                        searchReturn = alphaBetaDepth1(st.reflectPure(), newBoard, moves, data, tele);
                    else 
                        searchReturn = alphaBetaInner<hasTranspositionTable>(st.reflectPure(), newBoard, depthleft-1, tableAccess, moves, data, tele, stop_token); 
                }
            }

            if (searchReturn.answer.returnState == SearchReturnState::SearchTerminated) {
                bestMove.answer.returnState    =   SearchReturnState::SearchTerminated;
                return bestMove;
            }

            searchReturn.answer.negate();


            if (searchReturn.answer.eval > bestMove.answer.eval) {
                bestMove.answer.eval= searchReturn.answer.eval;
                bestMove.answer.bestMove = mv;
                bestMove.bestMoveIdx = idx;
                bestMove.hasBestMoveIdx = true;

                std::println("Depthleft {} eval {}", depthleft, searchReturn.answer.eval);
            }

            if (bestMove.answer.eval > st.beta) {
                bestMove.answer.returnState = SearchReturnState::BetaCutoff;
                break;
            }

            st.alpha = std::max(st.alpha, bestMove.answer.eval);
        }

        if (bestMove.answer.returnState != SearchReturnState::BetaCutoff) {
            auto currentDepth = st.rootSearchDepth - depthleft;
            auto &pvNode = tele->principleVariation[currentDepth];
            auto &pvBoard= tele->boardStates[currentDepth];
            pvNode = bestMove.answer;
            pvBoard = board.applyMovePure(pvNode.bestMove);
        }


        std::println("Depth {}, returning", depthleft);

        return bestMove;
    }
}



template <EvalFunction eval>
inline searchAnswer 
alphaBeta(int depth, chessBoard board, transpositionTableAccess tableAccess, searchTelemetry* tele, std::optional<std::stop_token> stop_token) {
    searchState st {-INF, INF, board.numPlys, static_cast<uint16_t>(depth)};
    stackStack218 allMoves;
    auto movegenData = chessMoves::makeAllMovesWithDataReturn(board, allMoves);
    std::pair<const stackStack218&, const chessMoves::movegenEngineData&> movegenRet {allMoves, movegenData};

    // testing without transposition table
    if (tableAccess.table != nullptr && tableAccess.interface != nullptr) {
        auto zHash = tableAccess.interface->hashPosition(board);
        auto& entry = tableAccess.interface->positionFromZHash(zHash, tableAccess.table);
        bool entryVerified = zHash == entry.zHash && allMoves.numitems() == entry.numMoves;
        auto* entryPtrVerified = entryVerified ? &entry : nullptr;

        return AlphaBeta::alphaBetaInner<true>(st, board, depth, tableAccess, allMoves, movegenData, tele, stop_token, entryPtrVerified).answer;
    } else {
        return AlphaBeta::alphaBetaInner<false>(st, board, depth, tableAccess, allMoves, movegenData, tele, stop_token, nullptr).answer;
    }
}

template 
searchAnswer 
alphaBeta<pieceWiseEval>(int depth, chessBoard board, transpositionTableAccess tableAccess, searchTelemetry* tele, std::optional<std::stop_token> stop_token);
