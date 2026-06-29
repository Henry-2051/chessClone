#include <cstdint>

#ifndef BOARD_STATE
#define BOARD_STATE
namespace board_state {

enum BoardState : uint8_t{
    EnPassantRight    = 0b00000010,
    WhiteTurn         = 0b00000100,
    WhiteLostCastlingRightsRight = 0b00001000,
    WhiteLostCastlingRightsLeft  = 0b00010000,
    BlacklostCastlingRightsRight = 0b00100000,
    BlackLostCastlingRightsLeft  = 0b01000000,
    HasEnPassant      = 0b10000000,
    VoidState         = 0
};

enum PawnState : uint8_t {
    PawnWhiteTurn = 0b100,
    PawnHasEnPassant = 0b010,
    PawnHasEnPassantRight = 0b001
};



inline uint8_t 
mapBoardToPawnState(uint8_t boardState) {
    return (boardState & WhiteTurn ? PawnWhiteTurn : 0) | (boardState & HasEnPassant ? PawnHasEnPassant: 0) | (boardState & EnPassantRight ? PawnHasEnPassantRight : 0);
}
}

#endif
