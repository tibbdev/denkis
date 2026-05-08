#include "serial_manager.h"
#include <iostream>

namespace serial_manager
{
    // Forward declaration of the recursive async read loop
    void start_async_read(SerialContext* ctx);

    SerialContext* connect(const std::string& port_name, unsigned int baud_rate)
    {
        SerialContext* ctx = new SerialContext();
        ctx->is_connected = false;
        ctx->rx_buffer.resize(1024); // 1KB chunks

        try
        {
            ctx->io_ctx = std::make_shared<asio::io_context>();
            ctx->work_guard = std::make_shared<asio::executor_work_guard<asio::io_context::executor_type>>(asio::make_work_guard(*ctx->io_ctx));
            ctx->port = std::make_shared<asio::serial_port>(*ctx->io_ctx);

            ctx->port->open(port_name);
            ctx->port->set_option(asio::serial_port_base::baud_rate(baud_rate));
            ctx->port->set_option(asio::serial_port_base::character_size(8));
            ctx->port->set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
            ctx->port->set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
            ctx->port->set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));

            ctx->is_connected = true;

            // Kick off the continuous background reading loop
            start_async_read(ctx);

            // Spin up the background thread to process Asio callbacks
            ctx->background_thread = std::thread([ctx]()
            {
                ctx->io_ctx->run();
            });
        }
        catch (const std::exception& e)
        {
            std::cerr << "Serial Connect Exception: " << e.what() << std::endl;
            disconnect(ctx);
            return nullptr;
        }

        return ctx;
    }

    void disconnect(SerialContext* ctx)
    {
        if (ctx == nullptr)
        {
            return;
        }

        ctx->is_connected = false;

        if (ctx->port && ctx->port->is_open())
        {
            ctx->port->cancel();
            ctx->port->close();
        }

        // Release the work guard so io_context.run() can finish
        if (ctx->work_guard)
        {
            ctx->work_guard.reset();
        }

        if (ctx->background_thread.joinable())
        {
            ctx->background_thread.join();
        }

        delete ctx;
    }

    void start_async_read(SerialContext* ctx)
    {
        if (!ctx->is_connected || !ctx->port->is_open())
        {
            return;
        }

        ctx->port->async_read_some(asio::buffer(ctx->rx_buffer),
            [ctx](const asio::error_code& error, size_t bytes_transferred)
            {
                if (!error && bytes_transferred > 0)
                {
                    // Lock the subscriber list
                    std::lock_guard<std::mutex> lock(ctx->subscribers_mtx);
                    
                    // Pipe the received bytes to all registered thread-safe buffers
                    for (ThreadSafeBuffer* buffer : ctx->subscribers)
                    {
                        std::lock_guard<std::mutex> buf_lock(buffer->mtx);
                        for (size_t i = 0; i < bytes_transferred; ++i)
                        {
                            buffer->data.push_back(ctx->rx_buffer[i]);
                        }
                    }

                    // Queue up the next read
                    start_async_read(ctx);
                }
                else if (error != asio::error::operation_aborted)
                {
                    std::cerr << "Serial Async Read Error: " << error.message() << std::endl;
                    ctx->is_connected = false;
                }
            });
    }

    void send(SerialContext* ctx, const uint8_t* data, size_t length)
    {
        if (ctx == nullptr || !ctx->is_connected)
        {
            return;
        }

        // asio::write is synchronous, but because we dispatch it to the io_context,
        // it runs safely on the background thread without blocking the UI.
        asio::post(*ctx->io_ctx, [ctx, data, length]()
        {
            try
            {
                asio::write(*ctx->port, asio::buffer(data, length));
            }
            catch (const std::exception& e)
            {
                std::cerr << "Serial Send Exception: " << e.what() << std::endl;
            }
        });
    }

    void send(SerialContext* ctx, const std::vector<uint8_t>& data)
    {
        // Overload for vectors, copying data into the lambda so it outlives the UI frame
        if (ctx == nullptr || !ctx->is_connected)
        {
            return;
        }

        asio::post(*ctx->io_ctx, [ctx, data]()
        {
            try
            {
                asio::write(*ctx->port, asio::buffer(data));
            }
            catch (const std::exception& e)
            {
                std::cerr << "Serial Send Exception: " << e.what() << std::endl;
            }
        });
    }

    void subscribe(SerialContext* ctx, ThreadSafeBuffer* buffer)
    {
        if (ctx == nullptr || buffer == nullptr)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(ctx->subscribers_mtx);
        ctx->subscribers.push_back(buffer);
    }

    void unsubscribe(SerialContext* ctx, ThreadSafeBuffer* buffer)
    {
        if (ctx == nullptr || buffer == nullptr)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(ctx->subscribers_mtx);
        for (auto it = ctx->subscribers.begin(); it != ctx->subscribers.end(); ++it)
        {
            if (*it == buffer)
            {
                ctx->subscribers.erase(it);
                break;
            }
        }
    }

    std::vector<uint8_t> read_all_bytes(ThreadSafeBuffer* buffer)
    {
        std::vector<uint8_t> output;

        if (buffer != nullptr)
        {
            std::lock_guard<std::mutex> lock(buffer->mtx);
            output.reserve(buffer->data.size());
            
            while (!buffer->data.empty())
            {
                output.push_back(buffer->data.front());
                buffer->data.pop_front();
            }
        }

        return output;
    }
}
