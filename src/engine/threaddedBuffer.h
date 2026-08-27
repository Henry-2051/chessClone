#include <atomic>
#include <condition_variable>
#include <mutex>
#include <print>
#include <string>
#include <vector>

// we are implementing a many producer single consumer architechture, where we want to minimise the lock contention of the producer so that we can maximise 
// the parrallelizability of our search, folling Amdahl's law

struct threaddedBuffer {
    std::mutex bufferMutex {};
    // we're essentially using a vector as a queue
    std::vector<std::string> buffer {}; 
};

struct interfacePrinterState{
    std::mutex consumerLock {};
    std::condition_variable flushBuffer {};

    threaddedBuffer stdoutBuffer {};
};

inline
void swapLogBuffers(std::vector<std::string>& buffer1, interfacePrinterState& threaddedInput) {
    std::unique_lock<std::mutex> consumerLock {threaddedInput.consumerLock};

    {
        std::lock_guard<std::mutex> bufferLock {threaddedInput.stdoutBuffer.bufferMutex};
        if (threaddedInput.stdoutBuffer.buffer.size() > 0) {
            buffer1.resize(0);
            threaddedInput.stdoutBuffer.buffer.swap(buffer1);
        }
    }
}


// consumer permenantly owns the interface printer state
template <size_t PollTimeMiliseconds>
void printerProcess(std::stop_token stopToken, interfacePrinterState& sharedState)  {
    std::vector<std::string> printItems {};
    // this approach recycles 2 arrays between 2 vectors to maintail very low lock contention 
    // the buffers are allowed to grow to avoid heap allocations, 
    std::unique_lock<std::mutex> printerLock {sharedState.consumerLock};
    while (!stopToken.stop_requested()) {
        sharedState.flushBuffer.wait_for(printerLock, std::chrono::milliseconds(PollTimeMiliseconds));

        {
            std::lock_guard<std::mutex> bufferLock {sharedState.stdoutBuffer.bufferMutex};
            sharedState.stdoutBuffer.buffer.swap(printItems);
        }

        for (auto item : printItems) {
            std::println("{}", item);
        }
        printItems.resize(0);
    }
}
