#include <algorithm>
#include <array>
#include <stdexcept>
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

    template <typename F>
    stackStack<T, mN>& stackTransorm(F transform) {
        for (int i = 0; i < currentNumberItems; ++i) {
            internalArray[i] = transform(internalArray[i]);
        }
        return *this;
    };
};
