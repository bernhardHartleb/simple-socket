#include <simple_socket/SimpleSocket.h>
#include <simple_socket/NetworkInterface.h>

#include <iostream>
#include <sstream>

const char usageString[] = " [ <interface> <setting> <value> ]\n"
"   \n"
"   If called without parameters, lists interfaces and their current settings.\n"
"   Changes said settings if called with parameters.\n"
"   \n"
"   Parameters:\n"
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
* since we should not automatically reconfigure a network
* interface each time we run tests
*
* Have a look at print() for how to get data, or at configure() for
* how to set data.
*/
int main( int argc, char* argv[])
{
	progName = argv[0];
	if(argc == 1) return print();
	if(argc == 4) {
		return configure(argv[1], argv[2], argv[3]);
	}
	usage();
	return -1;
}

//! just print program usage
void usage()
{
	std::cout << progName << usageString;
}

//! output information for all accessible interfaces
int print()
{
	auto ifs = NET::getNetworkInterfaces();
	// go through all settings and print them
	for(const auto& i : ifs)
	{
		std::cout << i.name() << ":" << "\n";
		std::cout << "\t" << "mtu " << i.getMTU() << "\n";
		std::cout << "\t" << "inet " << i.getInterfaceAddress() << " ";
		std::cout         << "netmask " << i.getNetmask() << " ";
		std::cout         << "broadcast " << i.getBroadcastAddress() << "\n";
		std::cout << "\t" << "ether " << i.getHardwareAddress() << std::endl;
	}
	return 0;
}

//! configure one setting of the interface
int configure(const std::string& interface, const std::string& setting, const std::string& val)
{
	// check which setting we want to configure
	// and set the value appropriately
	try {
		NET::NetworkInterface nwi(interface);
		if(setting == "mtu") {
			nwi.setMTU(std::stoi(val));
		} else if(setting == "address") {
			nwi.setInterfaceAddress(val);
		} else if(setting == "broadcast") {
			nwi.setBroadcastAddress(val);
		} else if(setting == "netmask") {
			nwi.setNetmask(val);
		} else if(setting == "destination") {
			nwi.setDestinationAddress(val);
		} else {
			usage();
			return -1;
		}
		return 0;
	}
	// since any of the calls might throw, catch errors
	// here and report them to the user
	catch(const NET::SocketException& e)
	{
		std::cerr << e.what() << " (" << e.errorCode() << ")" << std::endl;
		return -1;
	}
}
