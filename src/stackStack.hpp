#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include "seperateBitboard.hpp"

#ifndef stack_stack
#define stack_stack


struct pieceMovement {
    uint64_t movement;
    uint8_t  pieceType;
};

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

enum IsCapture : uint8_t {
    NotCapture = 0b0000,
    Capture    = 0b0001
};

template<typename T, std::size_t mN>
struct stackStack
{
    std::array<T, mN> internalArray;
    std::size_t capacity;
    std::size_t currentNumberItems;

    stackStack(std::array<T, mN> initialArray, std::size_t numElements)
      : internalArray(initialArray)
      , capacity(mN)
      , currentNumberItems(numElements) {
        if (numElements > mN) { throw std::invalid_argument("passed top element index cant be greater than the capacity of the stack");}
    };

    T pop() {
        -- currentNumberItems;
        if (currentNumberItems >= capacity) {throw std::underflow_error("stack underflow, no more elements to pop"); }
        T result = internalArray[currentNumberItems];
        internalArray[currentNumberItems] = T{};
        return result;
    };

    stackStack<T, mN>& push(T value) {
        if (currentNumberItems == capacity) { throw std::overflow_error("stack overflow, trying to push while at capacity"); }
        internalArray[currentNumberItems] = value;
        ++currentNumberItems;
        return *this;
    }

    template<std::size_t P>
    stackStack<T, mN>& pushItems(std::array<T, P> item_array, std::size_t numberOfItems) {
        std::size_t spaceLeft = capacity - currentNumberItems; 
        if (numberOfItems > spaceLeft) {throw std::overflow_error("stack overflow, trying to push too many items onto the stackStack"); }
        for (std::size_t i = 0; i < numberOfItems; ++ i) {
            push(item_array[i]);
        }
        return *this;
    }

    template<std::size_t P>
    stackStack<T, mN>& pushItems(stackStack<T, P> itemsStack) {
        return pushItems(itemsStack.internalArray, itemsStack.currentNumberItems);
    }

    bool isEmpty() {
        return currentNumberItems == 0;
    }

    T& top() {
        if (currentNumberItems == 0) { throw std::underflow_error("trying to get the top of an empty stack"); }

        return internalArray[currentNumberItems-1];
    }

    template <typename Function>
    stackStack<T, mN>& stackTransorm(Function transform) {
        for (int i = 0; i < currentNumberItems; ++i) {
            internalArray[i] = transform(internalArray[i]);
        }
        return *this;
    };
};

using stackStack218 = stackStack<pieceMovement, 218>;

struct singleColorChessMoveStack : stackStack218 {
    uint64_t attacked_squares{};
    singleColorChessMoveStack(std::array<pieceMovement, 218> initialArray, size_t numElements) 
    : stackStack(initialArray, numElements) {};
    
    singleColorChessMoveStack pushMoves(uint64_t moved_piece, uint64_t moves, uint8_t piece_type) {
        std::pair<std::array<uint64_t, 28>, std::size_t> seperation_result = seperateBitboard<28>(moves);
        size_t numMoves = seperation_result.second;
        std::array<uint64_t, 28>& moves_array = seperation_result.first;
        
        for (size_t i = 0; i < numMoves; i++) {
            uint64_t move = moves_array[i] | moved_piece;
            pieceMovement toPush{move, piece_type};
            push(toPush);
        }
        return *this;
    }
};

template <size_t Cap>
stackStack<uint64_t, Cap>
seperateBitboardIntoStack(uint64_t pieces) {
    std::pair<std::array<uint64_t, Cap>, std::size_t> result = seperateBitboard<Cap>(pieces); 
    return stackStack(result.first, result.second);
}
#endif
