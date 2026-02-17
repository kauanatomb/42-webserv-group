#include "network/Connection.hpp"
#include <sys/socket.h>
#include <iostream>

Connection::Connection(int fd, const RuntimeConfig& config) 
    : _socket_fd(fd), 
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
    (void)_config;
    (void)_parser;
    char buffer[4096];
    ssize_t bytes = recv(_socket_fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
        _state = CLOSED;
        return;
    }
    _read_buffer.append(buffer, bytes);
    
    _state = PARSING;
    if (_parser.parse(_read_buffer, _request)) {
        _state = HANDLING;
        _request.print();

    //     RequestHandler handler(_config);
    //     _response = handler.handle(_request);
    //     _write_buffer = _response.toString();
    //     _state = ConnectionState::WRITING;
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
