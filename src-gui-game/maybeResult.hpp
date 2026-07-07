#ifndef MAYBE_RESULT_H
#define MAYBE_RESULT_H

#include <iostream>

template<typename T>
struct maybeResult{
    bool m_isNothing;
    T m_value;

public:
    maybeResult() : m_isNothing(true) {};

    maybeResult(T val) : m_isNothing(false), m_value(val) {};

    maybeResult<T>
    setValue(T val) {
        m_value = val;
        m_isNothing = false;
        return *this;
    }

    void print() {
        if (m_isNothing) {
            std::cout << "Nothing" << std::endl;
        } else {
            std::cout << "Value: "  << m_value << std::endl;
        }
    }

    T getValue() const {
        if (not m_isNothing) {
            return m_value;
        } else {
            throw std::runtime_error("Trying to get the value of Nothing\n");
        }
    }

    bool exists() const {
        return not m_isNothing;
    }

    // operator to convert to a different type
    // template <typename U>
    // requires std::convertible_to<T, U>
    // operator maybeResult<U>() const {
    //     if (!m_isNothing) {
    //         return maybeResult<U>(static_cast<U>(m_value));
    //     } else {
    //         return maybeResult<U>();
    //     }
    // }
};
#endif // MAYBE_RESULT_H
