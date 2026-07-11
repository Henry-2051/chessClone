
#include <cstddef>
#include <stdexcept>
#include <vector>

// work in progress / curiosity project, currently using std::stack for the gui

template<typename T>
struct DynamicStack
{
    std::vector<T> internalVector;
    std::size_t currentNumberItems;
    typedef T* iterator;
    typedef const T* const_iterator;

    DynamicStack() : internalVector({}), currentNumberItems(0) {};

    T* begin() {
        return internalVector.begin();
    }

    T* end() {
        return internalVector.begin() + currentNumberItems;
    }

    const T* begin() const {
        return internalVector.data();
    }

    const T* end() const {
        return internalVector.data() + currentNumberItems;
    }

    T pop() {
        if (currentNumberItems == 0) {
            throw std::underflow_error("stack underflow, no more elements to pop");
        }
        T value {internalVector[currentNumberItems-1]};
        -- currentNumberItems;
        internalVector.resize(currentNumberItems);
        return value;
    };

    T peek() const {
        if (currentNumberItems == 0) {
            throw std::logic_error("trying to peek a stack which has no elements");
        }
        return internalVector[currentNumberItems-1];
    }

    DynamicStack& resizeToElement(T* elementIterator) {
        T* start = begin();
        size_t newNumElements = elementIterator -start;
    }

    DynamicStack<T>& push(T value) {
        internalVector.push_back() = value;
        ++currentNumberItems;
        return *this;
    }


    bool isEmpty() const {
        return currentNumberItems == 0;
    }

    size_t numitems() const {
        return currentNumberItems;
    }
};
