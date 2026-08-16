#include "timer.hpp"


template<Timers T>
std::map<Timers, std::string> Timer<T>::timerNames {
    {Timers::AddToStack218, "AddToStack218"},
    {Timers::SlidingAttackBishop, "SlidingAttackBishop"},
    {Timers::SlidingAttackRook, "SlidingAttackRook"},
    {Timers::KingMoveFunction, "KingMoveFunction"},
    {Timers::ComputeCheckMasks, "ComputeCheckMasks"},
    {Timers::ComputePinMasks, "ComputePinMasks"},
    {Timers::MakeAllMoves, "MakeAllMoves"},
    {Timers::SeperateBitboardFastStack, "SeperateBitboardFastStack"},
    {Timers::QueenAttack, "QueenAttack"},
    {Timers::AttackCreationBishop, "AttackCreationBishop"},
    {Timers::AttackCreationRook, "AttackCreationRook"},
};

template struct Timer<Timers::AddToStack218>;
template struct Timer<Timers::ComputeCheckMasks>;
template struct Timer<Timers::ComputePinMasks>;
template struct Timer<Timers::KingMoveFunction>;
template struct Timer<Timers::SlidingAttackBishop>;
template struct Timer<Timers::SlidingAttackRook>;
template struct Timer<Timers::MakeAllMoves>;
template struct Timer<Timers::SeperateBitboardFastStack>;
template struct Timer<Timers::QueenAttack>;
template struct Timer<Timers::AttackCreationRook>;
template struct Timer<Timers::AttackCreationBishop>;


template<Timers T>
long Timer<T>::timeInNanoseconds {0};

template<Timers T>
long Timer<T>::numEvents{0};

template<Timers T>
Timer<T>::Timer() {
    local_start = std::chrono::steady_clock::now();
}

template<Timers T>
Timer<T>::~Timer() {
    auto stop = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - local_start).count();

    numEvents ++;
    timeInNanoseconds += duration;
}

template<Timers T>
void Timer<T>::printAverageTimeMilliseconds() {
    std::println("{} took {} ms on average over {} trials", timerNames[T], Timer<T>::timeInNanoseconds / (Timer<T>::numEvents * 1000000uz), Timer<T>::numEvents);
}

template<Timers T>
void Timer<T>::printAverageTimeMicroseconds() {
    std::println("{} took {} us on average over {} trials", timerNames[T], Timer<T>::timeInNanoseconds / (Timer<T>::numEvents * 1000uz), Timer<T>::numEvents);
}

template<Timers T>
void Timer<T>::printAverageTimeNanoseconds() {
    std::println("{} took {} ns on average over {} trials", timerNames[T], Timer<T>::timeInNanoseconds / Timer<T>::numEvents, Timer<T>::numEvents);
}

template<Timers T>
void Timer<T>::printTotalTimeNanoseconds() {
    std::println("{} took {} ns in total", timerNames[T], Timer<T>::timeInNanoseconds);
};

template<Timers T>
void Timer<T>::printTotalTimeMicroseconds() {
    std::println("{} took {} us in total", timerNames[T], Timer<T>::timeInNanoseconds / 1000.0);
};

template<Timers T>
void Timer<T>::printTotalTimeMilliseconds() {
    std::println("{} took {} ms in total", timerNames[T], Timer<T>::timeInNanoseconds / 1000000.0);
};
