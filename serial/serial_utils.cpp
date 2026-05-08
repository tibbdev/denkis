#include "serial_utils.h"

#if defined(_WIN32)

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>

std::vector<SerialPortInfo> enumerate_serial_ports()
{
    std::vector<SerialPortInfo> ports;
    
    // Get a handle to the device information set for all present serial ports
    HDEVINFO device_info_set = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
    
    if (device_info_set == INVALID_HANDLE_VALUE)
    {
        return ports;
    }

    SP_DEVINFO_DATA device_info_data;
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
    
    DWORD device_index = 0;

    // Iterate through all devices in the set
    while (SetupDiEnumDeviceInfo(device_info_set, device_index, &device_info_data))
    {
        device_index++;

        // Get the registry key for the device
        HKEY device_key = SetupDiOpenDevRegKey(device_info_set, &device_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        
        if (device_key == INVALID_HANDLE_VALUE)
        {
            continue;
        }

        char port_name[256] = {0};
        DWORD port_name_size = sizeof(port_name);
        DWORD reg_type = 0;

        // Query the "PortName" value from the registry
        if (RegQueryValueExA(device_key, "PortName", NULL, &reg_type, (LPBYTE)port_name, &port_name_size) == ERROR_SUCCESS)
        {
            SerialPortInfo info;
            info.port_name = std::string(port_name);

            // Now get the friendly name (description) from the device setup API
            char friendly_name[256] = {0};
            DWORD friendly_name_size = sizeof(friendly_name);
            
            if (SetupDiGetDeviceRegistryPropertyA(device_info_set, &device_info_data, SPDRP_FRIENDLYNAME, NULL, (PBYTE)friendly_name, friendly_name_size, NULL))
            {
                info.description = std::string(friendly_name);
            }
            else
            {
                info.description = "Unknown Serial Device";
            }

            ports.push_back(info);
        }

        RegCloseKey(device_key);
    }

    SetupDiDestroyDeviceInfoList(device_info_set);

    return ports;
}

#elif defined(__APPLE__)

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>

std::vector<SerialPortInfo> enumerate_serial_ports()
{
    std::vector<SerialPortInfo> ports;
    
    mach_port_t master_port;
    kern_return_t kern_result = IOMasterPort(MACH_PORT_NULL, &master_port);
    
    if (kern_result != KERN_SUCCESS)
    {
        return ports;
    }

    CFMutableDictionaryRef classes_to_match = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classes_to_match == NULL)
    {
        return ports;
    }

    io_iterator_t matching_services;
    kern_result = IOServiceGetMatchingServices(master_port, classes_to_match, &matching_services);
    
    if (kern_result != KERN_SUCCESS)
    {
        return ports;
    }

    io_object_t modem_service;
    
    while ((modem_service = IOIteratorNext(matching_services)))
    {
        SerialPortInfo info;
        
        // Get the callout device path (e.g., /dev/cu.usbserial)
        CFTypeRef bsd_path_as_cfstring = IORegistryEntryCreateCFProperty(modem_service, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
        
        if (bsd_path_as_cfstring)
        {
            char path_cstring[256];
            if (CFStringGetCString((CFStringRef)bsd_path_as_cfstring, path_cstring, sizeof(path_cstring), kCFStringEncodingUTF8))
            {
                info.port_name = std::string(path_cstring);
            }
            CFRelease(bsd_path_as_cfstring);
        }

        // Get the product name for the description
        CFTypeRef product_name_as_cfstring = IORegistryEntrySearchCFProperty(modem_service, kIOServicePlane, CFSTR("Product Name"), kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents);
        
        if (product_name_as_cfstring)
        {
            char name_cstring[256];
            if (CFStringGetCString((CFStringRef)product_name_as_cfstring, name_cstring, sizeof(name_cstring), kCFStringEncodingUTF8))
            {
                info.description = std::string(name_cstring);
            }
            CFRelease(product_name_as_cfstring);
        }
        else
        {
            info.description = "Unknown Serial Device";
        }

        if (!info.port_name.empty())
        {
            ports.push_back(info);
        }

        IOObjectRelease(modem_service);
    }

    IOObjectRelease(matching_services);
    
    return ports;
}

#elif defined(__linux__)

#include <filesystem>
#include <fstream>
#include <iostream>

std::vector<SerialPortInfo> enumerate_serial_ports()
{
    std::vector<SerialPortInfo> ports;
    namespace fs = std::filesystem;

    const std::string sys_tty_path = "/sys/class/tty";
    
    if (!fs::exists(sys_tty_path))
    {
        return ports;
    }

    // Iterate through all TTY devices in sysfs
    for (const auto& entry : fs::directory_iterator(sys_tty_path))
    {
        std::string device_name = entry.path().filename().string();
        
        // Construct the path to the physical device symlink
        fs::path device_path = entry.path() / "device";
        
        // If there is no device symlink, it's a virtual TTY (like a standard console), so we skip it
        if (!fs::exists(device_path))
        {
            continue;
        }

        SerialPortInfo info;
        info.port_name = "/dev/" + device_name;
        info.description = "Unknown Serial Device";

        // Attempt to read the hardware ID/vendor string for the description
        // This usually lives in the parent device of the tty driver block
        fs::path subsystem_path = device_path / "subsystem";
        
        if (fs::exists(subsystem_path))
        {
            std::string subsystem_name = fs::read_symlink(subsystem_path).filename().string();
            
            if (subsystem_name == "usb-serial" || subsystem_name == "usb")
            {
                fs::path vendor_path = device_path / "../../manufacturer";
                fs::path product_path = device_path / "../../product";
                
                std::string description = "";
                
                if (fs::exists(vendor_path))
                {
                    std::ifstream vendor_file(vendor_path);
                    std::string vendor;
                    std::getline(vendor_file, vendor);
                    description += vendor + " ";
                }
                
                if (fs::exists(product_path))
                {
                    std::ifstream product_file(product_path);
                    std::string product;
                    std::getline(product_file, product);
                    description += product;
                }
                
                if (!description.empty())
                {
                    info.description = description;
                }
            }
            else if (subsystem_name == "pnp")
            {
                info.description = "Built-in Serial Port";
            }
        }

        ports.push_back(info);
    }

    return ports;
}

#else
#error "Unsupported OS for serial port enumeration"
#endif
