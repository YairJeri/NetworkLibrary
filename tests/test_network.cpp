#include <iostream>
#include <cassert>
#include <string>
#include "network.h"

enum class TestMsg : uint32_t {
    Ping,
    Text,
    ComplexData
};

struct PlayerState {
    uint32_t id;
    float x;
    float y;
    int health;
};

void TestThreadSafeQueue() {
    std::cout << "[TEST] Running ThreadSafeQueue tests... ";
    network::ThreadSafeQueue<int> q;
    assert(q.empty());
    assert(q.count() == 0);

    q.push_back(10);
    q.push_back(20);
    q.push_front(5);

    assert(q.count() == 3);
    assert(q.front() == 5);
    assert(q.back() == 20);

    assert(q.pop_front() == 5);
    assert(q.pop_back() == 20);
    assert(q.pop_front() == 10);
    assert(q.empty());
    std::cout << "PASSED\n";
}

void TestThreadSafeMap() {
    std::cout << "[TEST] Running ThreadSafeMap tests... ";
    network::ThreadSafeMap<uint32_t, std::string> map;
    assert(map.empty());

    map.set(1, "PlayerOne");
    map.set(2, "PlayerTwo");

    assert(map.size() == 2);
    auto val1 = map.get(1);
    assert(val1.has_value() && val1.value() == "PlayerOne");

    map.erase(1);
    assert(!map.get(1).has_value());
    assert(map.size() == 1);
    std::cout << "PASSED\n";
}

void TestPackageSerialization() {
    std::cout << "[TEST] Running Package serialization tests... ";
    network::package<TestMsg> pkg;
    pkg.header.id = TestMsg::ComplexData;

    PlayerState out_player{ 42, 125.5f, -30.0f, 100 };
    std::string out_name = "Player_Hero";

    pkg << out_player;
    pkg << out_name;

    std::string in_name;
    PlayerState in_player{};

    pkg >> in_name;
    pkg >> in_player;

    assert(in_name == "Player_Hero");
    assert(in_player.id == 42);
    assert(in_player.x == 125.5f);
    assert(in_player.y == -30.0f);
    assert(in_player.health == 100);
    assert(pkg.body.empty());

    // Probar protección contra underflow
    bool caught_exception = false;
    try {
        int dummy = 0;
        pkg >> dummy;
    } catch (const std::runtime_error&) {
        caught_exception = true;
    }
    assert(caught_exception);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Running Unit Tests ===\n";
    TestThreadSafeQueue();
    TestThreadSafeMap();
    TestPackageSerialization();
    std::cout << "=== All tests completed successfully! ===\n";
    return 0;
}
