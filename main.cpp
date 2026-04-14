#include <iostream>
#include <string>
using namespace std;

#include "CircularQueueSPSC.h"
#include "ZeroCopyRingBuffer.h"

// int main() {
//     CircularQueue<int> q(4);
//     q.print();
//     cout<<"---isEmpty " << q.isEmpty() << endl;
//     q.enQueue(1);
//     q.enQueuePowerOfTwo(2);
//     cout<<"---isFull " << q.isFull() << endl;
//     q.enQueuePowerOfTwo(3);
//     cout<<"---isFull " << q.isFull() << endl;
//     return 0;
// }


// int main() {
//     ZeroCopyRingBuffer<int, 8> buffer; // Size must be a power of two
//     // Producer side: reserve, write, commit
//     int* writeSlot = buffer.reserve_slot();
//     if (writeSlot) {
//         *writeSlot = 123;
//         buffer.commit_slot();
//     } else {
//         std::cout << "---Buffer is full\n";
//     }
//     // Consumer side: peek, read, release
//     int* readSlot = buffer.peek_slot();
//     if (readSlot) {
//         std::cout << "--Read value: " << *readSlot << "\n";
//         buffer.release_slot();
//     } else {
//         std::cout << "-- Buffer is empty\n";
//     }
//     return 0;
// }

#include <thread>
#include <chrono>

// A typical market data update struct
struct MarketData {
    uint32_t symbol_id;
    double price;
    int32_t volume;
};

int main() {
    // 1. Initialize the buffer (Size must be power of 2)
    ZeroCopyRingBuffer<MarketData, 4> ring_buffer;
    std::atomic<bool> running{true};

    // 2. PRODUCER THREAD: Simulates incoming network packets
    std::thread producer([&]() {
        for (int i = 0; i < 3; ++i) {
            // Claim a slot in the buffer
            MarketData* slot = nullptr;
            int spin_count = 0;
            while ((slot = ring_buffer.reserve_slot()) == nullptr) {
                // Buffer full - in HFT we usually spin (busy-wait)
                spin_count++;
                std::cout << "-- [Producer] spin_count: " << spin_count << std::endl; 
                if (spin_count < 1000) {
                    // Spin for a short time
                    for (volatile int j = 0; j < 100; ++j) {}
                } else {
                    // Yield if spinning too long
                    std::this_thread::yield();
                    spin_count = 0;
                    std::cout << "--[Producer] Yield if spinning too long " << std::endl; 
                }
            }

            // Write DIRECTLY into the buffer memory (Zero-Copy)
            std::cout << "[Producer] Symbol: " << slot->symbol_id 
                        << " | Price: " << slot->price 
                        << " | Vol: " << slot->volume << "\n";
            slot->symbol_id = 42;
            slot->price = 150.0 + (i * 0.5);
            slot->volume = 100 + i;

            // Make the data visible to the consumer
            ring_buffer.commit_slot();

            std::this_thread::sleep_for(std::chrono::milliseconds(220));
        }
        std::cout << "--[Producer] running -> false " << std::endl; 
        running = false;
    });

    // 3. CONSUMER THREAD: Simulates your trading strategy logic
    std::thread consumer([&]() {
        while (running || ring_buffer.peek_slot() != nullptr) {
            // Peek at the next available data
            MarketData* data = ring_buffer.peek_slot();
            
            if (data) {
                // Access the data directly from the buffer
                std::cout << "[Consumer] Symbol: " << data->symbol_id 
                          << " | Price: " << data->price 
                          << " | Vol: " << data->volume << "\n";

                // Mark the slot as free for the producer to reuse
                ring_buffer.release_slot();
            } else {
                // Buffer empty - spin or yield
                std::this_thread::yield();
                // std::cout << "--- [Consumer] Buffer empty -> Yield, running:" << running << std::endl; 
            }
        }
    });

    producer.join();
    consumer.join();

    std::cout << "--end Data processing complete." << std::endl;
    return 0;
}