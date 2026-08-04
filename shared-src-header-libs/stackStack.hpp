// #include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <print>
#include <cstdint>
#include <format>
#include <functional>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <iostream>
#include "boardState.hpp"
#include <map>
#include "helpers.hpp"


#ifndef stack_stack
#define stack_stack



inline uint8_t PieceTypeMask = 0b111;
inline uint8_t isCaptureMask = 0b1000;

enum PieceType : uint8_t {
    Pawn = 0b0,
    Rook = 0b1,
    Knight = 0b10,
    Bishop = 0b11,
    Queen = 0b100,
    King = 0b101,
    NotAPiece = 0b1000,
};

// enum PieceTypePlusOne : uint8_t {
//     Pawn = 0b1,
//     Rook =
// }

inline std::string_view getPieceTypeString(PieceType pt) {
    switch (pt) {
        case (PieceType::Bishop):
            return "Bishop";
        case (PieceType::King):
            return "King";
        case (PieceType::Knight):
            return "Knight";
        case (PieceType::Pawn):
            return "Pawn";
        case (PieceType::Queen):
            return "Queen";
        case (PieceType::Rook):
            return "Rook";
        case (PieceType::NotAPiece):
            return "NotAPiece";
    };
    return "Unrecognised input in getPieceTypeString";
}

template <PieceType pt>
concept SlidingPiece =
    pt == PieceType::Rook ||
    pt == PieceType::Bishop ||
    pt == PieceType::Queen;

enum IsCapture : uint8_t {
    NotCapture = 0b0000,
    Capture    = 0b0001
};



struct pieceMovement {
    uint64_t  movement;
    uint64_t  secondMovement;
    // Bitboard 1 {White, Black}, Bitboard 2 {White, Black}
    PieceType movement1WhiteBB {NotAPiece};
    PieceType movement1BlackBB {NotAPiece};
    PieceType movement2WhiteBB {NotAPiece};
    PieceType movement2BlackBB {NotAPiece};
    int8_t enPassantState{-1}; // en passant behavior
                               //
                               // represents the square that can be en passant captured into
                               // if blacks a pawn moves 2 squares and our pawn was on b5 then 
                               // an en passant capture square would be generated on b6 
                               // values from 0-63 represent en passant capture 
                               // -1 represents no en passant capture, everything else is an error 
                               //
                               // when we apply the move we simply xor, using the xor proprties of 
                               // commutivity and that they cancel out, we can encode previous en passant 
                               // states where moves work to update the boards state in a fully reversible way
                               // meaning board.applyMove(mv).applyMove(mv) === board for any valid move 
    
    uint8_t boardStateChange {board_state::WhiteTurn};

    
    //TODO halfmove and fullmove clock

    bool compareForSelection(const pieceMovement& rval) const {
        // were going to be hacky and only compare the first part unless its pawn promoiton
        if (std::popcount(movement) == 1 && std::popcount(rval.movement) == 1) {
            return (movement | secondMovement) == (rval.movement | rval.secondMovement) && 
                movement2WhiteBB == rval.movement2WhiteBB && movement2BlackBB == rval.movement2BlackBB;
        }

        return movement == rval.movement;
    }

    // pieceMovement operator|(const pieceMovement& rval) const{
    //     if (pieceType != rval.pieceType) {
    //         throw std::logic_error("trying to merge two piece movements with different piece type values");
    //     }
    //
    //     if (has_second_movement && rval.has_second_movement && (secondPieceType != rval.secondPieceType)) {
    //         throw std::logic_error("trying to merge two piece movements with different SECOND piece type values");
    //     }
    //
    //     return 
    //     {
    //         movement | rval.movement, 
    //         second_optional_movement | rval.second_optional_movement, 
    //         pieceType, 
    //         has_second_movement ? secondPieceType : rval.secondPieceType, 
    //         has_second_movement || rval.has_second_movement
    //     };
    // }

    void printThis() const {
        std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
        
        std::println("First movement");
        helpers::printBitboard(this->movement);
        std::println("second movement");
        helpers::printBitboard(secondMovement);

        std::println("first  movement black bitboard to apply to {}", getPieceTypeString(movement1BlackBB));
        std::println("first  movement white bitboard to apply to {}", getPieceTypeString(movement1WhiteBB));
        std::println("second movement black bitboard to apply to {}", getPieceTypeString(movement2BlackBB));
        std::println("second movement white bitboard to apply to {}", getPieceTypeString(movement2WhiteBB));

        std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
        }
};



inline void printCustomStruct(const pieceMovement& movement) {
    movement.printThis();
}


template<typename T, std::size_t mN>
struct FastStack
{
    std::array<T, mN> internalArray;
    std::size_t currentNumberItems;
    typedef T* iterator;
    typedef const T* const_iterator;

    FastStack() : internalArray({}), currentNumberItems(0) {};

    FastStack(const std::array<T, mN>& initialArray, std::size_t numElements): internalArray(initialArray), currentNumberItems(numElements) {
        if (numElements > mN) { throw std::invalid_argument("passed top element index cant be greater than the capacity of the stack");}
    };

    T* begin() {
        return internalArray.data();
    }

    T* end() {
        return internalArray.data() + currentNumberItems;
    }

    T& operator[](size_t idx) {
        return internalArray[idx];
    }

    const T& operator[](size_t idx) const {
        return internalArray[idx];
    }

    const T* begin() const {
        return internalArray.data();
    }

    const T* end() const {
        return internalArray.data() + currentNumberItems;
    }


    T pop() {
        if (currentNumberItems == 0) {
            throw std::underflow_error("stack underflow, no more elements to pop");
        }
        -- currentNumberItems;
        return internalArray[currentNumberItems];
    };

    T peek() const {
        if (currentNumberItems == 0) {
            throw std::logic_error("trying to peek a stack which has no elements, nothing to see here");
        }
        return internalArray[currentNumberItems-1];
    }

    inline
    FastStack<T, mN>& push(T&& value) {
        if (currentNumberItems == mN) { 
            std::println("overflow error readout : FastStack<{}, {}>", typeid(T).name(), mN);
            std::println("trying to push item");
            if constexpr (typeid(T) == typeid(uint64_t)) {
                helpers::printBitboard(value);
            }
            throw std::overflow_error("stack overflow, trying to push while at capacity"); 
        }
        internalArray[currentNumberItems] = value;
        ++currentNumberItems;
        return *this;
    }

    inline
    FastStack<T, mN>& pushVal(T value) {
        if (currentNumberItems == mN) { 
            std::println("overflow error readout : FastStack<{}, {}>", typeid(T).name(), mN);
            std::println("trying to push item");
            if constexpr (typeid(T) == typeid(uint64_t)) {
                helpers::printBitboard(value);
            }
            throw std::overflow_error("stack overflow, trying to push while at capacity"); 
        }
        internalArray[currentNumberItems] = value;
        ++currentNumberItems;
        return *this;
    }

    // inline
    // FastStack<T, mN>& push(std::optional<T> value) {
    //     if (value.has_value())
    //         return push(*value);
    //     return *this;
    // }

    template<std::size_t P>
    FastStack<T, mN>& pushItems(const std::array<T, P>& item_array, std::size_t numberOfItems) {
        // std::size_t spaceLeft = mN - currentNumberItems; 
        // if (numberOfItems > spaceLeft) {throw std::overflow_error("stack overflow, trying to push too many items onto the stackStack"); }
        for (std::size_t i = 0; i < numberOfItems; ++ i) {
            push(item_array[i]);
        }
        return *this;
    }


    template<std::size_t P>
    FastStack<T, mN>& pushItems(const FastStack<T, P>& itemsStack) {
        return pushItems(itemsStack.internalArray, itemsStack.currentNumberItems);
    }

    inline bool isEmpty() const {
        return currentNumberItems == 0;
    }

    inline bool notEmpty() const {
        return currentNumberItems != 0;
    }

    size_t numitems() const {
        return currentNumberItems;
    }

    T& top() {
        if (currentNumberItems == 0) { throw std::underflow_error("trying to get the top of an empty stack"); }

        return internalArray[currentNumberItems-1];
    }

    template <typename Function>
    FastStack<T, mN>& stackTransorm(Function transform) {
        for (auto i {0uz}; i < currentNumberItems; ++i) {
            internalArray[i] = transform(internalArray[i]);
        }
        return *this;
    };
};

using stackStack218 = FastStack<pieceMovement, 218>;

 


// struct singleColorChessMoveStack : stackStack218 {
//     uint64_t attacked_squares{};
//     singleColorChessMoveStack(std::array<pieceMovement, 218> initialArray, size_t numElements) 
//     : FastStack(initialArray, numElements) {};
//
//     singleColorChessMoveStack pushMoves(uint64_t moved_piece, uint64_t moves, uint8_t piece_type, board_state::BoardState bstate) {
//         std::pair<std::array<uint64_t, 28>, std::size_t> seperation_result = seperateBitboard<28>(moves);
//         size_t numMoves = seperation_result.second;
//         std::array<uint64_t, 28>& moves_array = seperation_result.first;
//
//         for (size_t i = 0; i < numMoves; i++) {
//             std::optional<pieceMovement> toPush {std::nullopt};
//             uint64_t move = moves_array[i] | moved_piece;
//
//             // its actually impossible to represent moves like castling and en passant( and promotion ) with just a single xor 
//             // operation on 
//             // a bitboard because if you try and capture en passant with a pawn you will end up with 2 pawns, we get around
//             // this by including a second move which erases the extra pawn. The down side of this is that our 
//             // piece movement struct goes from 16 bytes to 24 bytes, but we can now represent every move
//             if ((bstate & board_state::HasEnPassant) && (piece_type == PieceType::Pawn)) {
//                 if (bstate & board_state::WhiteTurn && 
//                         (moved_piece >> 7 == moves_array[i] || moved_piece >> 9 == moves_array[i])) {
//                     uint64_t epp_capture = moves_array[i] << 8;
//                     move |= epp_capture;
//                     toPush = {{move, (epp_capture), PieceType(piece_type), PieceType(piece_type), true}};
//                 } else if (moved_piece << 7 == moves_array[i] || moved_piece << 9 == moves_array[i]) {
//                     uint64_t epp_capture = moves_array[i] >> 8;
//                     move |= epp_capture;
//                     toPush = {move, (epp_capture), PieceType(piece_type), PieceType(piece_type), true};
//                 }
//             } 
//             auto standardMovement = [move, piece_type]()->std::optional<pieceMovement>{ 
//                 return {{move, 0, PieceType(piece_type), PieceType(piece_type), false}}; 
//             };
//             push(toPush.or_else(standardMovement).value());
//         }
//         return *this;
//     }
// };


#endif
