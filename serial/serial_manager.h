#pragma once

#include <asio.hpp>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>
#include <memory>
#include <cstdint>
#include <string>

// A thread-safe wrapper around a deque for receiving bytes
struct ThreadSafeBuffer
{
    std::deque<uint8_t> data;
    std::mutex mtx;
};

// Holds the state of a single serial connection
struct SerialContext
{
    std::shared_ptr<asio::io_context> io_ctx;
    std::shared_ptr<asio::executor_work_guard<asio::io_context::executor_type>> work_guard;
    std::shared_ptr<asio::serial_port> port;
    std::thread background_thread;
    
    // Multi-buffer dispatch list
    std::vector<ThreadSafeBuffer*> subscribers;
    std::mutex subscribers_mtx;
    
    // Internal Asio read buffer
    std::vector<uint8_t> rx_buffer;
    bool is_connected;
};

namespace serial_manager
{
    // Connection Management
    SerialContext* connect(const std::string& port_name, unsigned int baud_rate);
    void disconnect(SerialContext* ctx);
    
    // Data Transmission
    void send(SerialContext* ctx, const uint8_t* data, size_t length);
    void send(SerialContext* ctx, const std::vector<uint8_t>& data);
    
    // Buffer Management
    void subscribe(SerialContext* ctx, ThreadSafeBuffer* buffer);
    void unsubscribe(SerialContext* ctx, ThreadSafeBuffer* buffer);
    
    // Utility to safely extract all current bytes from a buffer
    std::vector<uint8_t> read_all_bytes(ThreadSafeBuffer* buffer);
}
