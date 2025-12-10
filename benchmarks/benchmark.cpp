#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include "network.h"

enum class BenchMsg : uint32_t {
    Payload
};

struct BenchmarkPacket {
    uint64_t sequence;
    uint64_t timestamp_ns;
    char payload[64];
};

int main() {
    std::cout << "===========================================\n";
    std::cout << "      Network Engine Performance Bench     \n";
    std::cout << "===========================================\n";

    constexpr size_t NUM_PACKETS = 500000;
    std::cout << "Measuring serialization/deserialization throughput (" << NUM_PACKETS << " pkts)...\n";

    network::package<BenchMsg> pkg;
    pkg.header.id = BenchMsg::Payload;

    BenchmarkPacket out_pkt{ 1, 123456789, "Benchmark test payload packet data" };

    auto start_ser = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < NUM_PACKETS; ++i) {
        pkg.reset();
        pkg << out_pkt;
    }
    auto end_ser = std::chrono::high_resolution_clock::now();

    double ser_seconds = std::chrono::duration<double>(end_ser - start_ser).count();
    double ser_throughput = static_cast<double>(NUM_PACKETS) / ser_seconds;

    std::cout << "  - Serialized " << NUM_PACKETS << " pkts in " << ser_seconds << " s ("
              << static_cast<uint64_t>(ser_throughput) << " pkts/sec)\n";

    // Medición de latencia de deserialización
    std::vector<double> latencies_ns;
    latencies_ns.reserve(NUM_PACKETS);

    BenchmarkPacket in_pkt{};
    for (size_t i = 0; i < NUM_PACKETS; ++i) {
        pkg.reset();
        pkg << out_pkt;

        auto t0 = std::chrono::high_resolution_clock::now();
        pkg >> in_pkt;
        auto t1 = std::chrono::high_resolution_clock::now();

        latencies_ns.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }

    std::sort(latencies_ns.begin(), latencies_ns.end());
    double p50 = latencies_ns[NUM_PACKETS * 0.50];
    double p95 = latencies_ns[NUM_PACKETS * 0.95];
    double p99 = latencies_ns[NUM_PACKETS * 0.99];

    std::cout << "  - Deserialization Latency:\n";
    std::cout << "      p50: " << p50 << " ns\n";
    std::cout << "      p95: " << p95 << " ns\n";
    std::cout << "      p99: " << p99 << " ns\n";

    // Medición de concurrencia en ThreadSafeQueue
    std::cout << "\nMeasuring ThreadSafeQueue concurrent throughput (4 producers, 1 consumer)...\n";
    network::ThreadSafeQueue<int> queue;
    constexpr int MSGS_PER_THREAD = 100000;
    constexpr int PRODUCERS = 4;
    const int TOTAL_MSGS = MSGS_PER_THREAD * PRODUCERS;

    auto start_queue = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> producers;
    for (int p = 0; p < PRODUCERS; ++p) {
        producers.emplace_back([&queue, p]() {
            for (int i = 0; i < MSGS_PER_THREAD; ++i) {
                queue.push_back(p * MSGS_PER_THREAD + i);
            }
        });
    }

    int consumed = 0;
    std::thread consumer([&queue, &consumed, TOTAL_MSGS]() {
        while (consumed < TOTAL_MSGS) {
            if (!queue.empty()) {
                queue.pop_front();
                ++consumed;
            } else {
                std::this_thread::yield();
            }
        }
    });

    for (auto& t : producers) t.join();
    consumer.join();

    auto end_queue = std::chrono::high_resolution_clock::now();
    double q_seconds = std::chrono::duration<double>(end_queue - start_queue).count();
    double q_throughput = static_cast<double>(TOTAL_MSGS) / q_seconds;

    std::cout << "  - Processed " << TOTAL_MSGS << " items in " << q_seconds << " s ("
              << static_cast<uint64_t>(q_throughput) << " ops/sec)\n";
    std::cout << "===========================================\n";

    return 0;
}
