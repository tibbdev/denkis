#include "serial/serial_manager.h"
#include <zenoh.hpp>

int main() {
    auto session = zenoh::expect(zenoh::open(zenoh::Config::create_default()));
    auto pub = session.declare_publisher("denkis/serial/rx");

    // Initialize your existing serial manager 
    auto* ctx = serial_manager::connect("COM3", 115200); 
    
    // Create a bridge: Asio -> Zenoh
    ThreadSafeBuffer rx_buf;
    serial_manager::subscribe(ctx, &rx_buf);

    while(true) {
        auto data = serial_manager::read_all_bytes(&rx_buf);
        if (!data.empty()) {
            pub.put(data); // Broadcast to anyone listening (GUI or Logic)
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
