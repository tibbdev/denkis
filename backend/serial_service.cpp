#include "serial/serial_manager.h"
#include "serial/serial_utils.h"
#include <zenoh.hpp>
#include <iostream>
#include <string>
#include <mutex>

// Configuration State
struct ServiceConfig
{
    std::string device_id = "default_bench";
    std::string port = "COM3";
    uint32_t baud = 1000000;
    std::mutex mtx;
} g_config;

int main(int argc, char** argv)
{
    // 1. Allow setting the name via command line: ./serial_service bench_01
    if (argc > 1)
    {
        g_config.device_id = argv[1];
    }

    std::cout << "Starting Serial Service: " << g_config.device_id << std::endl;

    // 2. Initialize Zenoh
    auto session = zenoh::expect(zenoh::open(zenoh::Config::create_default()));

    // Paths follow the pattern: denkis/{id}/serial/{rx|tx|config|info}
    std::string base_path = "denkis/" + g_config.device_id + "/serial";
    
    auto pub_rx = session.declare_publisher(base_path + "/rx");
    
    // Global context pointer - we wrap it so the config subscriber can swap it
    SerialContext* serial_ctx = serial_manager::connect(g_config.port, g_config.baud);
    ThreadSafeBuffer rx_buf;
    serial_manager::subscribe(serial_ctx, &rx_buf);

    // 3. Command Listener: Handles GUI requests to change Port/Baud
    // GUI sends: "COM4:9600"
    auto sub_config = session.declare_subscriber(base_path + "/config", [&](const zenoh::Sample& s) 
    {
        std::string cmd = s.get_payload().as_string();
        size_t sep = cmd.find(':');
        if (sep != std::string::npos)
        {
            std::string new_port = cmd.substr(0, sep);
            uint32_t new_baud = std::stoi(cmd.substr(sep + 1));

            std::lock_guard<std::mutex> lock(g_config.mtx);
            std::cout << "Reconfiguring to " << new_port << " @ " << new_baud << std::endl;
            
            if (serial_ctx != nullptr)
            {
                serial_manager::disconnect(serial_ctx);
            }

            g_config.port = new_port;
            g_config.baud = new_baud;

            serial_ctx = serial_manager::connect(new_port, new_baud);
            serial_manager::subscribe(serial_ctx, &rx_buf);
        }
    });

    // 4. TX Listener: GUI sends data here to be written to the serial port
    auto sub_tx = session.declare_subscriber(base_path + "/tx", [&](const zenoh::Sample& s) 
    {
        auto payload = s.get_payload().as_string();
        std::lock_guard<std::mutex> lock(g_config.mtx);
        if (serial_ctx != nullptr && serial_ctx->is_connected)
        {
            serial_manager::send(serial_ctx, payload);
        }
    });

    // 5. Discovery Provider: Responds to wildcard queries like "denkis/*/serial/info"
    auto queryable = session.declare_queryable(base_path + "/info", [&](const zenoh::Query& q) 
    {
        std::lock_guard<std::mutex> lock(g_config.mtx);
        std::string info = "ID=" + g_config.device_id + 
                           ";Port=" + g_config.port + 
                           ";Baud=" + std::to_string(g_config.baud);
        
        q.reply(zenoh::Sample(q.get_keyexpr(), info));
    });

    // 6. Main Loop: Bridge Asio RX buffer to Zenoh Publication
    std::cout << "Service active on " << base_path << std::endl;

    while (true)
    {
        auto data = serial_manager::read_all_bytes(&rx_buf);
        if (!data.empty())
        {
            // Convert vector<uint8_t> to string/bytes for Zenoh
            pub_rx.put(std::string(data.begin(), data.end()));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return 0;
}