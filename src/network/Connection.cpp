#include "network/Connection.hpp"
#include "resolver/HandlerResolver.hpp"
#include <sys/socket.h>
#include <iostream>

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
    char buffer[4096];
    ssize_t bytes = recv(_socket_fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
        _state = CLOSED;
        return;
    }
    _read_buffer.append(buffer, bytes);
    _state = PARSING;
    if (_parser.parse(_read_buffer, _request)) {
        // if (_parser.getHasError()) {
            // int status = _parser.getErrorStatus();
            // _response = HttpResponse::fromStatus(status);
        // } else {
            
        //     const RuntimeLocation* loc = ServerResolver::resolve(_config, _socket_key, _request);
        //     if (!loc) {
        //         _response = HttpResponse::fromStatus(500);
        //     } else {
        //         RequestHandler handler(*loc);
        //         _response = handler.handle(_request);
        //         (void)loc; // TODO: remove when handler is implemented
        //     }
        // }
        //_write_buffer = _response.serialize();
        //_state = WRITING;
    }
    _request.print();
}

void Connection::onWritable() {
    ssize_t bytes = send(_socket_fd, _write_buffer.c_str(), _write_buffer.size(), 0);
    if (bytes <= 0) {
        _state = CLOSED;
        return;
    }
    _write_buffer.erase(0, bytes);
    if (_write_buffer.empty()) {
        // if (_keep_alive) {
        //     resetForNextRequest();
        // } else {
            _state = CLOSED;
        // }
    }
}
