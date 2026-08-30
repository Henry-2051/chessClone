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
#include <stdexcept>


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


//needs some work
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

int
noMovesEvalulation(const chessBoard& board, const chessMoves::movegenEngineData& mgenData) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t ourKing = board.pieceToBitboardConst<PieceType::King>(isWhiteTurn);
    // std::println("nomoves");
    // we are checkmated
    if (mgenData.enemyAttacksMushed & ourKing) {
        // we would rather be mated in 3 plys than 1 ply
        
        // we score getting checkmated at a greater depth to be better, this means that the engine
        // always mates in as few moves as possible
        
        return (-pMaps::pieceScoreArray[PieceType::King].score) + board.numPlys;
    } else {
        // a draw is a draw
        return 0;
    }

}

// play out the most promising tactical move until there are no more promising tactical moves
// since we are just playing out a capture chain we can discard every move other than the one 
// we choose
template<EvalFunction eval>
quiessenceSearchReturn
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

    if (allMoves.numitems() == 0)
        return {noMovesEvalulation(board, metadata), SearchReturnState::EndOfGame};

    if (bestMoveScore != 0 && bestMoveScore > 10) {
        // auto mvString = board.uciStringMove(allMoves[bestMoveIdx]);
        // std::println("playing {}", board.uciStringMove(allMoves[bestMoveIdx]));
        
        // we dont know whether the capture is good, we may choose to play a quiet move instead
        // this relies on the assumption that there exists a move which preserves all our material on the board
        // the idea of this is that its going to increase the strength of the engine more than it decrements it
        // futhermore we can exploit bitboards to write an optimised movegen that only considers captures

        auto q_ret = quiessenceSearch<eval>(board.applyMovePure(allMoves[bestMoveIdx]), allMovesMemory, false);
        q_ret.score = -q_ret.score;
        // maybe capturing that haning piece led to checkmate, an ideal maximising player would not play that move and 
        // instead stabalise the position
        if (q_ret.score > staticEvalScore)
            return q_ret;
        else 
            return {staticEvalScore, SearchReturnState::Normal};

    } else if (!_madeNullMove) {
        pieceMovement nullMove = pieceMovement{0,0, NotAPiece, NotAPiece, NotAPiece, NotAPiece, static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)), board.enPassantState, board_state::WhiteTurn};


        auto q_ret = quiessenceSearch<eval>(board.applyMovePure(nullMove), allMovesMemory, true);

        q_ret.score = -q_ret.score;
        // maybe capturing that haning piece led to checkmate, an ideal maximising player would not play that move and 
        // instead stabalise the position
        if (q_ret.score > staticEvalScore)
            return q_ret;
        else 
            return {staticEvalScore, SearchReturnState::Normal};
    }

    return {staticEvalScore, SearchReturnState::Normal};
};

template 
quiessenceSearchReturn search::quiessenceSearch<pieceWiseEval>(chessBoard board, stackStack218& allMovesMemory, bool _madeNullMove);

namespace AlphaBeta {

// int negaMaxEval (const chessBoard& boardBefore, const pieceMovement& mv, uint64_t enemyAttacksMushed) {
//     // bool isWhiteTurn = boardBefore.m_board_state & board_state::WhiteTurn;
//
//     stackStack218 allMoveMem;
//     return search::quiessenceSearch<pieceWiseEval>(boardBefore.applyMovePure(mv), allMoveMem);
//     // return (isWhiteTurn) ? pieceWiseEval(boardBefore.applyMovePure(mv)) : -pieceWiseEval(boardBefore.applyMovePure(mv));
//
//     // if (!isQuietAfterMove(boardBefore, mv, enemyAttacksMushed)) 
//     //     return search::quiessenceSearch<pieceWiseEval>(boardBefore.applyMovePure(mv), allMoveMem);
//     // else 
//     //     return (!isWhiteTurn) ? pieceWiseEval(boardBefore.applyMovePure(mv)) : -pieceWiseEval(boardBefore.applyMovePure(mv));
// }


searchAnswerInternal alphaBetaDepth1(searchState st, const chessBoard& board, const stackStack218& movesForBoard, const chessMoves::movegenEngineData& mgenData, searchTelemetry* tele = nullptr) { 
    if (movesForBoard.numitems() == 0) {
        throw std::logic_error("alpha beta depth 1 cannot be called with empty move array");
    }


    searchAnswerInternal bestMove {searchAnswer{-INF, {}, SearchReturnState::NotAssigned}};
    bool betaCutoff = false;


    if (st.alpha > st.beta)
        throw std::logic_error("error alpha cannot be greater than beta, invalid program state");

    stackStack218 allMovesMem;
    for (const auto& mv : movesForBoard) {
        
        chessBoard newBoard = board.applyMovePure(mv);
        // int score = -negaMaxEval(board, mv, mgenData.enemyAttacksMushed);

        auto q_ret = search::quiessenceSearch<pieceWiseEval>(newBoard, allMovesMem);
        q_ret.score = -q_ret.score;

        tele->nodes ++;
        
        if (q_ret.score > bestMove.answer.eval) {
            bestMove.answer = searchAnswer{q_ret.score, mv, q_ret.rState};
        }

        if (q_ret.score >= st.beta) {
            betaCutoff = true;
            break;
        }

        // if (score >= st.beta) {
        //     bestMove.answer.returnState = SearchReturnState::BetaCutoff;
        //     break;
        // }
        //
        // if (score > st.alpha) {
        //     st.alpha = score;
        // }
    }

    if (!betaCutoff) {
        auto pvArrStart = tele->pvMemoryStart;
        auto currentEntryOffset = pvMemoryOffset(tele->searchDepth, 1);
        *(pvArrStart + currentEntryOffset) = bestMove.answer;
        // std::println("pv changed(1) : \n {}, offset = {}", board.uciStringMove(bestMove.answer.bestMove), currentEntryOffset);
    }

    return bestMove;
}

    template <bool hasTranspositionTable>
    searchAnswerInternal 
    alphaBetaInner(searchState st, chessBoard board, int depthleft, transpositionTableAccess& tableAccess, 
            const stackStack218& allMoves, const chessMoves::movegenEngineData& mgendata, 
            searchTelemetry* tele=nullptr, std::optional<std::stop_token> stop_token = {}, 
            TT::tableEntry* currentTTEntryVerified = nullptr, bool useAlphaBetaPruning = true) 
    { 
        if (allMoves.numitems() == 0) {
            return searchAnswerInternal{searchAnswer{noMovesEvalulation(board, mgendata), {}, SearchReturnState::EndOfGame}};
        }
        if (stop_token.has_value() && stop_token->stop_requested()) {
            return searchAnswerInternal{searchAnswer{-INF, {}, SearchReturnState::SearchTerminated}};
        }

        // std::array<ttEntryCopy, 218> ttEntries{};

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
        searchAnswerInternal bestMove {searchAnswer{-INF, {}, SearchReturnState::NotAssigned}};

        searchState dummyST {st};

        bool betaCutoff = false;
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

                    if (moves.numitems() == 0) {
                        int noMovesEval = noMovesEvalulation(newBoard, data);
                        searchReturn.answer = searchAnswer{noMovesEval, {}, SearchReturnState::EndOfGame};
                    }
                    else if (depthleft <= 2) {
                        searchReturn = alphaBetaDepth1(st.reflectPure(), newBoard, moves, data, tele);
                    }
                    else 
                        searchReturn = alphaBetaInner<hasTranspositionTable>(st.reflectPure(), newBoard, depthleft-1, tableAccess, moves, data, tele, stop_token, currentTTEntryVerified, useAlphaBetaPruning); 
                }
            }

            if (searchReturn.answer.returnState == SearchReturnState::SearchTerminated) {
                bestMove.answer.returnState    =   SearchReturnState::SearchTerminated;
                return bestMove;
            }

            searchReturn.answer.negate();

            // if (depthleft == 6) {
            //     std::println("candiate eval {}, current eval {}, {}", searchReturn.answer.eval, bestMove.answer.eval, board.uciStringMove(mv));
            // }

            if (searchReturn.answer.eval > bestMove.answer.eval && (searchReturn.answer.returnState != SearchReturnState::NotAssigned && searchReturn.answer.returnState != SearchReturnState::SearchTerminated)) {
                bestMove.answer.returnState = searchReturn.answer.returnState;
                bestMove.answer.eval= searchReturn.answer.eval;
                bestMove.answer.bestMove = mv;
                bestMove.bestMoveIdx = idx;
                bestMove.hasBestMoveIdx = true;
            }

            if (useAlphaBetaPruning) {
                if (searchReturn.answer.eval >= st.beta) {
                    betaCutoff = true;
                    break;
                }

                st.alpha = std::max(st.alpha, bestMove.answer.eval);
            }
        }

        if (!betaCutoff && bestMove.answer.returnState != SearchReturnState::NotAssigned) {
            searchAnswer* begin = tele->pvMemoryStart;

            size_t currentNode  = pvMemoryOffset(tele->searchDepth, depthleft);
            size_t previousNode = pvMemoryOffset(tele->searchDepth, depthleft - 1);
            // std::println("{}, {}, {}, {}", tele->searchDepth, depthleft, currentNode, previousNode);

            std::copy(begin + previousNode, begin + (previousNode + depthleft - 1), begin + (currentNode + 1));
            *(begin + currentNode) = bestMove.answer;
            // std::println("pv changed : \n {}, offset = {}", board.uciStringMove(bestMove.answer.bestMove), currentNode);
        }

        return bestMove;
    }
}



searchAnswer 
alphaBeta(int depth, chessBoard board, transpositionTableAccess tableAccess, searchTelemetry* tele, std::optional<std::stop_token> stop_token, bool useAlphaBetaPruning) {
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

        return AlphaBeta::alphaBetaInner<true>(st, board, depth, tableAccess, allMoves, movegenData, tele, stop_token, entryPtrVerified, useAlphaBetaPruning).answer;
    } else {
        return AlphaBeta::alphaBetaInner<false>(st, board, depth, tableAccess, allMoves, movegenData, tele, stop_token, nullptr, useAlphaBetaPruning).answer;
    }
}
