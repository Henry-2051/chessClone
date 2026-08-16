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

// our consumer which will live in a seperate thread

template <size_t PollTimeMiliseconds>
void printerProcess(std::stop_token stopToken, interfacePrinterState& sharedState)  {
    std::unique_lock<std::mutex> printerLock {sharedState.consumerLock};
    while (!stopToken.stop_requested()) {
        sharedState.flushBuffer.wait_for(printerLock, std::chrono::milliseconds(PollTimeMiliseconds));

        std::vector<std::string> printItems;

        {
            std::lock_guard<std::mutex> bufferLock {sharedState.stdoutBuffer.bufferMutex};
            auto numItems {sharedState.stdoutBuffer.buffer.size()};

            for (auto i {numItems}; i -- > 0;) {
                printItems.push_back(std::move(sharedState.stdoutBuffer.buffer[i]));
            }

            sharedState.stdoutBuffer.buffer.resize(0);
        }

        for (auto item : printItems) {
            std::println("{}", item);
        }
    }
}
