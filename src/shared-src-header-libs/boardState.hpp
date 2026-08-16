#include <cstdint>

#ifndef BOARD_STATE
#define BOARD_STATE
namespace board_state {

// this encodes the square behind the pawn that has moved 2 places, ie the en passant capturable square,
// valid values are between 0 and 63, when this isnt available the value will be negative
using enPassantState = int8_t;

enum BoardState : uint8_t{
    WhiteTurn         = 0b00000100,
    WhiteLostCastlingRightsRight = 0b00001000,
    WhiteLostCastlingRightsLeft  = 0b00010000,
    BlacklostCastlingRightsRight = 0b00100000,
    BlackLostCastlingRightsLeft  = 0b01000000,
    VoidState         = 0
};

static const uint8_t allCastlingFields_const = WhiteLostCastlingRightsLeft | WhiteLostCastlingRightsRight | BlackLostCastlingRightsLeft | BlacklostCastlingRightsRight;


}

#endif
