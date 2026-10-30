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
#include <simple_socket/TCPSocket.h>

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
        NET::TCPSocket client = server.accept();

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

To use the library in your own project, it is easiest to use add_subdirectory in your CMakeLists.txt:

```cmake
add_subdirectory(external/simple-socket)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE simple-socket)
```

When used as a subproject, simple-socket does not build tests or examples by default.
For a standalone build with unit tests, install CppUnit and use:

```sh
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

SocketCAN and SCTP support can be enabled with `-DSIMPLE_SOCKET_ENABLE_CAN=ON` and `-DSIMPLE_SOCKET_ENABLE_SCTP=ON` respectively.
API documentation can be generated with the `doc` target when Doxygen is available.

## License

This library is released under the MIT License.
See `LICENSE`.
