#include "../SimpleSocket.h"
#include "../SocketUtils.h"
#include <iostream>
#include <string>
#include <sstream>

const char usageString[] = " [ <interface> <setting> <value> ]\n"
"   \n"
"   If called without parameters, lists interfaces and their current settings.\n"
"   Changes said settings if called with parameters.\n"
"   \n"
"   Paramters:\n"
"       interface   name of the interface to configure\n"
"       setting     name of the setting to change\n"
"       value       new value for said setting\n"
"   \n"
"   Settings:\n"
"       mtu         maximum transmission unit\n"
"       address     IPv4 interface address\n"
"       broadcast   IPv4 broadcast address\n"
"       netmask     IPV4 netmask\n"
"       destination IPv4 destination address\n";

void usage();
int print();
int configure(const std::string& interface, const std::string& setting, const std::string& val);

char* progName;

/*!
 * \brief small program with ifconfig like behaviour
 * 
 * Shows the usage of SocketUtils, also used to test these
 * since we should not e.g. atomatically reconfigure a network
 * interface each time we run tests
 * 
 * Have a look at print() for how to get data, or at configure() for
 * how to set data.
 */
int main( int argc, char* argv[])
{
    progName = argv[0];
    if(argc == 1)
    {
        return print();
    } else if(argc == 4) {
        return configure(argv[1], argv[2], argv[3]);
    }
    usage();
    return -1;
}

//! just print program usage
void usage() {
    std::cout << progName << usageString;
}

// small wrapper that converts exceptions into a <fail> string
template<typename Callable>
std::string wrapEx(Callable c)
{
    try {
        std::ostringstream ss;
        ss << c();
        return ss.str();
    } catch(const NET::SocketException& e)
    {
        return "<fail>";
    }
}

//! output information for all accessible interfaces
int print() {
    // get a list of all available interfaces
    auto ifs = NET::getNetworkInterfaces();

    // do through all settings and print them
    // since all of the calls might throw, all of them are
    // wrapped in wrapEx for this example
    for(const auto& i : ifs)
    {
        std::cout << i << ":" << std::endl;
        std::cout << "\t" << "mtu " << wrapEx([i] {return NET::getMTU(i);}) << std::endl;
        std::cout << "\t" << "ip " << wrapEx([i] {return NET::getInterfaceAddress(i);}) << " ";
        std::cout         << "netmask " << wrapEx([i] {return NET::getNetmask(i);}) << " ";
        std::cout         << "broadcast " << wrapEx([i] {return NET::getBroadcastAddress(i);}) << " " << std::endl;
        std::cout << "\t" << "ether " << wrapEx([i] {return NET::getHardwareAddress(i);}) << std::endl;
    }

    return 0;
}

//! configure one setting of the interface
int configure(const std::string& interface, const std::string& setting, const std::string& val) {
    // check which setting we want to configure
    // and set the value appropriately
    try {
        if(setting == "mtu")
        {
            NET::setMTU(interface, stoi(val));
        } else if(setting == "address")
        {
            NET::setInterfaceAddress(interface, val);
        } else if(setting == "broadcast")
        {
            NET::setBroadcastAddress(interface, val);
        } else if(setting == "netmask")
        {
            NET::setNetmask(interface, val);
        } else if(setting == "destination")
        {
            NET::setDestinationAddress(interface, val);
        } else
        {
            usage();
            return -1;
        }
        return 0;
    // since any of the calls might throw, catch errors
    // here and report them to the user
    } catch(const NET::SocketException& e)
    {
        std::cerr << e.what() << " (" << e.errorCode() << ")" << std::endl;
        return -1;
    }
}
