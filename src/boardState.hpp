#include <cstdint>

#ifndef BOARD_STATE
#define BOARD_STATE
namespace board_state {

enum BoardState : uint8_t{
    WhiteTurn         = 0b000001,
    WhiteCastledRight = 0b000010,
    WhiteCastledLeft  = 0b000100,
    BlackCastledRight = 0b001000,
    BlackCastledLeft  = 0b010000,
    HasEnPassant      = 0b100000,
    VoidState         = 0
};

enum PawnState : uint8_t {
    PawnWhiteTurn = 0b10,
    PawnHasEnPassant = 0b01
};

inline uint8_t 
mapBoardToPawnState(uint8_t boardState) {
    return (boardState & WhiteTurn ? 0b10 : 0b00) | (boardState & HasEnPassant ? 0b01 : 0b00);
}
}

#endif
