# simple-socket

`simple-socket` is a lightweight C++17 based wrapper around POSIX socket APIs.
It keeps the native socket model visible while providing RAII ownership, exceptions and convenience functions.
All IP addresses are simple strings. Blocking socket operations have variants accepting a timeout.
Utilities for DNS resolution and limited network interface management are provided.

## Features

- IPv4/IPv6 TCP and UDP sockets
- UDP multicast support (IPv4 only)
- Unix-domain stream and datagram sockets
- Blocking I/O plus timeout variants for connect, accept and receive
- Peer-disconnect tracking for connected sockets
- Stream based sendAll and receiveAll helpers
- Hostname, service and network-interface utilities
- Optional SocketCAN support
- Optional SCTP support
- CMake build, unit tests and example programs

## Documentation

- [API documentation](https://bernhardhartleb.github.io/simple-socket/doxygen/html/index.html)
- [Use-case examples](https://github.com/bernhardHartleb/simple-socket/tree/master/examples)

## Example

Minimal TCP server:

```cpp
#include "TCPSocket.h"

#include <iostream>
#include <utility>

int main()
{
    try
    {
        NET::TCPSocket server;
        server.setReuseAddress(true);
        server.bind("127.0.0.1", 47777);
        server.listen();

        std::cout << "waiting for a client\n";
        NET::TCPSocket::Handle handle = server.accept();
        NET::TCPSocket client(std::move(handle));

        const char message[] = "hello from simple-socket\n";
        client.sendAll(message, sizeof(message) - 1);
    }
    catch (const NET::SocketException& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
```

## Building

A basic build without unit tests:

```sh
cmake -S . -B build -DBUILD_TESTING=OFF
cmake --build build
```

To build and run the unit tests, install CppUnit and use:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

SocketCAN and SCTP support can be enabled with `-DBUILD_CAN=ON` and `-DBUILD_SCTP=ON` respectively.
API documentation can be generated with the `doc` target when Doxygen is available.

## License

MIT. See `LICENSE`.
