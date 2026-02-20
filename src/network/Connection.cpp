#include "network/Connection.hpp"
#include "resolver/ServerResolver.hpp"
#ifdef DEBUG_LOG
#include "resolver/SocketKeyUtils.hpp" //for debuging
#include <iostream> //for debugging
#endif
#include <sys/socket.h>

Connection::Connection(int fd, const RuntimeConfig& config, const SocketKey& socket_key) 
    : _socket_fd(fd), 
    _socket_key(socket_key),
    _read_buffer(),
    _write_buffer(),
    _config(config),
    _state(READING),
    _keep_alive(false) {}


bool Connection::wantsWrite() const {
    return _state == WRITING;
}

bool Connection::isClosed() const {
    return _state == CLOSED;
}

void Connection::onReadable() {
    (void)_keep_alive;
    (void)_socket_key;
    (void)_config;
    //(void)_parser;
    char buffer[4096];
    ssize_t bytes = recv(_socket_fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
        _state = CLOSED;
        return;
    }
    _read_buffer.append(buffer, bytes); 
    #ifdef STUB_RESPONSE
     _write_buffer = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "Connection: close\r\n"
        "\r\n"
        "OK";
    _state = WRITING;
    return;
    #endif
    //_state = PARSING;
    // if (_parser.parse(_read_buffer, _request)) {
    //     _state = HANDLING;
    //     _request.print();

    //     RequestHandler handler(_config);
    //     _response = handler.handle(_request);
    //     _write_buffer = _response.toString();
    //     _state = ConnectionState::WRITING;
    //}
}


void Connection::onWritable() {
    ssize_t bytes = send(_socket_fd, _write_buffer.c_str(), _write_buffer.size(), 0);
    if (bytes <= 0) {
        _state = CLOSED;
        return;
    }
    _write_buffer.erase(0, bytes);
    if (_write_buffer.empty()) 
    {
        _state = CLOSED;
    }
}

// void Connection::onWritable() {
//     ssize_t bytes = send(_socket_fd, _write_buffer.c_str(), _write_buffer.size(), 0);
//     if (bytes <= 0) {
//         _state = ConnectionState::CLOSED;
//         return;
//     }
//     _write_buffer.erase(0, bytes);
//     if (_write_buffer.empty()) {
//         if (_keep_alive) {
//             resetForNextRequest();
//         } else {
//             _state = ConnectionState::CLOSED;
//         }
//     }
// }


