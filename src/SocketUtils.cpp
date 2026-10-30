#include "SocketUtils.h"
#include "SimpleSocket.h"

#include <arpa/inet.h>
#include <netdb.h>

#include <cstring>
#include <memory>

using namespace NET;

std::string NET::resolveHostname( const std::string& hostname)
{
	addrinfo hints = {};
	hints.ai_family = AF_INET;

	addrinfo* result = nullptr;
	int status = getaddrinfo( hostname.c_str(), nullptr, &hints, &result);
	if( status != 0 || result == nullptr)
		throw SocketException("Failed to resolve address (getaddrinfo)", false);

	std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)>
		result_guard(result, &::freeaddrinfo);

	if( result->ai_addr == nullptr || result->ai_addrlen < sizeof(sockaddr_in))
		throw SocketException("Resolver returned an invalid IPv4 address", false);

	sockaddr_in addr = {};
	std::memcpy( &addr, result->ai_addr, sizeof(addr));
	return inet_ntoa( addr.sin_addr);
}

uint16_t NET::resolveService( const std::string& service, const std::string& protocol)
{
	struct servent* serv;
	if( protocol.empty())
		serv = getservbyname( service.c_str(), nullptr);
	else
		serv = getservbyname( service.c_str(), protocol.c_str());

	if(serv)
		return ntohs( static_cast<uint16_t>(serv->s_port));
	return 0;
}
