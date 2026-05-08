#pragma once

#include <string>
#include <vector>

// Struct to hold the enumerated port data
struct SerialPortInfo
{
    std::string port_name;
    std::string description;
};

// Cross-platform function to retrieve available serial ports
std::vector<SerialPortInfo> enumerate_serial_ports();
