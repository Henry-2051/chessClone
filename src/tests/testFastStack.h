#include "stackStack.hpp"
#include "doctest.h"

// A helper function to build a default stackStack from an std::array.
template<typename T, std::size_t mN>
FastStack<T, mN> makeStack(const std::array<T, mN>& arr, std::size_t topIndex) {
    return FastStack<T, mN>(arr, topIndex);
};

TEST_CASE("Valid initialization") {
    std::array<int, 5> arr = {0, 0, 0, 0, 0};
    // Provide a valid top element index less than capacity.
    REQUIRE_NOTHROW( makeStack(arr, 0) );
}

TEST_CASE("Invalid initialization with too high top index") {
    std::array<int, 5> arr = {0, 0, 0, 0, 0};
    // Since top index equals capacity, this should throw.
    REQUIRE_THROWS_AS( makeStack(arr, 6), std::invalid_argument );
}

TEST_CASE("Push elements works until full") {
    std::array<int, 3> arr = {0, 0, 0};
    auto stack = makeStack(arr, 0);
    // Push first element.
    REQUIRE_NOTHROW( stack.pushVal(10) );
    // Push second element.
    REQUIRE_NOTHROW( stack.pushVal(20) );
    // Push third element.
    REQUIRE_NOTHROW( stack.pushVal(30) );
    // Stack should now be full, so pushing one more element should throw.
    REQUIRE_THROWS_AS( stack.pushVal(40), std::overflow_error );
}

TEST_CASE("Pop element returns the correct value") {
    std::array<int, 3> arr = {1, 2, 3};
    // Initialize with three elements.
    auto stack = makeStack(arr, 3);
    // Pop should return the last element pushed.
    int value = stack.pop();
    REQUIRE( value == 3 );
    // And then the top should now be the second element.
    value = stack.pop();
    REQUIRE( value == 2 );
}

TEST_CASE("Pop from empty stack throws underflow") {
    std::array<int, 2> arr = {10, 20};
    auto stack = makeStack(arr, 0);  // start with empty stack
    REQUIRE_THROWS_AS( stack.pop(), std::underflow_error );
}

TEST_CASE("PushItems with array works correctly") {
    std::array<int, 5> baseArr = {0, 0, 0, 0, 0};
    auto stack = makeStack(baseArr, 0);
    std::array<int, 3> items = {1, 2, 3};

    // Should work if there's room.
    REQUIRE_NOTHROW( stack.pushItems(items, 3) );
    // Now there are 3 items; pushing an array of 3 should exceed capacity.
    std::array<int, 3> tooMany = {4, 5, 6};
    REQUIRE_THROWS_AS( stack.pushItems(tooMany, 3), std::overflow_error );
}

TEST_CASE("PushItems with another stackStack works correctly") {
    std::array<int, 5> baseArr = {0, 0, 0, 0, 0};
    auto mainStack = makeStack(baseArr, 0);
    
    // Create another stackStack with two items.
    std::array<int, 3> arrayForStack = {7, 8, 0};
    auto anotherStack = makeStack(arrayForStack, 2); // Has two valid items
    
    // Pushing from anotherStack into mainStack should work if there's room.
    REQUIRE_NOTHROW( mainStack.pushItems(anotherStack) );
}
