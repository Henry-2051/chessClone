#include "transpositionTable.h"
#include <cstdint>
#include <random>
using namespace TT;

transpositionTableInterface::transpositionTableInterface(int tableSizeMB, uint64_t seed) 
    :   
        tableSize{static_cast<size_t>((tableSizeMB * 1024 * 1024) / 16)}
{
    std::mt19937_64 mt64 {seed};
    for (auto& squareArray : pieceSquareRNG){
        for (auto& square : squareArray) {
            square = mt64();
        }
    }
    whiteToMoveRNG = mt64();
    for (auto& cr : castlingRightsRNG) {
        cr = mt64();
    }
    for (auto& epp : enPassantFileRNG) {
        epp = mt64();
    }
}

uint64_t
transpositionTableInterface::hashPosition(const chessBoard& board) const {
    uint64_t zHash {0};
    for (size_t ctr{6}; ctr -- > 0;) {
        uint64_t whitePieces {board.getPiecesByColorConst(true)[ctr]};
        uint64_t blackPieces {board.getPiecesByColorConst(false)[ctr]};
        while (whitePieces != 0 && blackPieces != 0) {
            size_t wPP = __builtin_ctzll(whitePieces);
            size_t bPP = __builtin_ctzll(blackPieces);
            whitePieces &= whitePieces -1;
            blackPieces &= blackPieces -1;

            zHash = zHash ^ (pieceSquareRNG[ctr][wPP] + pieceSquareRNG[ctr][bPP]);
        }

        // minimise the number of loop operations, if one of the bitboards is non zero after the add whitePieces will 
        // be equal to the non zero one
        // in practice this will cut our iterations down by a factor of 2, with a maximum number of iterations of 10 
        // all pawns promote to (R | B | N) --> 10 x ( R | B | K ) 
        // hikaru has checkmated with 5 bishops, but I havent seen anyone checkmate with 10 bishops
        
        whitePieces += blackPieces;

        while (whitePieces != 0) {
            size_t wPP = __builtin_ctzll(whitePieces);
            whitePieces &= whitePieces -1;
            zHash = zHash ^ pieceSquareRNG[ctr][wPP];
        }
    }

    // enum BoardState : uint8_t{
    //     WhiteTurn         =            0b00000100,
    //     WhiteLostCastlingRightsRight = 0b00001000,
    //     WhiteLostCastlingRightsLeft  = 0b00010000,
    //     BlacklostCastlingRightsRight = 0b00100000,
    //     BlackLostCastlingRightsLeft  = 0b01000000,
    //     VoidState         = 0
    // };
    
    uint8_t isWhiteTurn {board.m_board_state};
    isWhiteTurn &= board_state::WhiteTurn;
    isWhiteTurn = isWhiteTurn >> 2;
    zHash = zHash ^ isWhiteTurn * whiteToMoveRNG;

    uint8_t castlingNumber {static_cast<uint8_t>(board.m_board_state)};
    castlingNumber &= board_state::allCastlingFields_const;
    castlingNumber = castlingNumber >> 3;
    zHash = zHash ^ castlingRightsRNG[castlingNumber];

    int8_t eppState {board.enPassantState};
    if (eppState < 0)
        return zHash;
    zHash = zHash ^ enPassantFileRNG[static_cast<size_t>(eppState % 8)];
    return zHash;
}
