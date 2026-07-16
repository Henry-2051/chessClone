#include <chrono>
#include <cstddef>
#include <string>
#include <map>
#include <ostream>
#include <print>
#pragma once

// timing has 90ns overhead, this is too much for measuring chess movegen
// switched to useing perf stat and perf report, increased performance by 70%
enum class Timers : int{
    AddToStack218 =0,
    SlidingAttackBishop,
    SlidingAttackRook,
    KingMoveFunction,
    ComputeCheckMasks,
    ComputePinMasks,
    MakeAllMoves,
    SeperateBitboardFastStack,
    QueenAttack,
    AttackCreationRook,
    AttackCreationBishop,
    __End_ENUM
};

template<Timers T>
struct Timer{
    // using TimeMap = std::map<char[3], std::chrono::time_point<std::chrono::nanoseconds>>;
    
    static std::map<Timers, std::string> timerNames;
    static double timeInNanoseconds;
    static long numEvents;

    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> local_start;

    Timer();

    ~Timer();

    static void printAverageTimeMilliseconds();

    static void printAverageTimeMicroseconds();

    static void printAverageTimeNanoseconds();

    static void printTotalTimeNanoseconds();
    static void printTotalTimeMicroseconds();
    static void printTotalTimeMilliseconds();
};
