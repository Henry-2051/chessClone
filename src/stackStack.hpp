// #include <algorithm>
#include <array>
#include <print>
#include <cstdint>
#include <format>
#include <functional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <iostream>
#include "boardState.hpp"
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
};

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
    };
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
    uint64_t  second_optional_movement;
    PieceType pieceType;
    PieceType secondPieceType;
    bool has_second_movement {false};

    bool operator==(const pieceMovement& rval) const {
        // if both movements have second movements then we want to compare them, this is the comparason in a lambda
        std::function<bool()> second = [&rval, this]->bool{
            return this->second_optional_movement == rval.second_optional_movement && this->secondPieceType == rval.secondPieceType;
        };
        bool secondMovement = has_second_movement && rval.has_second_movement;

        return movement == rval.movement && pieceType == rval.pieceType && (!secondMovement || second());
    }

    pieceMovement operator|(const pieceMovement& rval) const{
        if (pieceType != rval.pieceType) {
            throw std::logic_error("trying to merge two piece movements with different piece type values");
        }

        if (has_second_movement && rval.has_second_movement && (secondPieceType != rval.secondPieceType)) {
            throw std::logic_error("trying to merge two piece movements with different SECOND piece type values");
        }

        return 
        {
            movement | rval.movement, 
            second_optional_movement | rval.second_optional_movement, 
            pieceType, 
            has_second_movement ? secondPieceType : rval.secondPieceType, 
            has_second_movement || rval.has_second_movement
        };
    }

    void printThis() const {
        std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
        std::cout << std::format("pieceType : {}\n", getPieceTypeString(this->pieceType));
        helpers::printBitboard(this->movement);

        if (this->has_second_movement) {
            std::cout << std::format("second pieceType : {}\n", getPieceTypeString(this->secondPieceType));
            helpers::printBitboard(this->second_optional_movement);
        }

        std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
        }
};



inline void printCustomStruct(const pieceMovement& movement) {
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
    std::cout << std::format("pieceType : {}\n", getPieceTypeString(movement.pieceType));
    helpers::printBitboard(movement.movement);

    if (movement.has_second_movement) {
        std::cout << std::format("second pieceType : {}\n", getPieceTypeString(movement.secondPieceType));
        helpers::printBitboard(movement.second_optional_movement);
    }

    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
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

    FastStack<T, mN>& push(T value) {
        if (currentNumberItems == mN) { throw std::overflow_error("stack overflow, trying to push while at capacity"); }
        internalArray[currentNumberItems] = value;
        ++currentNumberItems;
        return *this;
    }

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

    bool isEmpty() const {
        return currentNumberItems == 0;
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
