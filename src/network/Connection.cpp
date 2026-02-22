#include "network/Connection.hpp"

//HTTP
#include "httpCore/HttpRequest.hpp"
#include "httpCore/HttpResponse.hpp"

//App logic
#include "network/RequestHandler.hpp"
#include "resolver/ServerResolver.hpp"

//system
#include <sys/socket.h>

#ifdef DEBUG_LOG
# include "resolver/SocketKeyUtils.hpp"
# include <iostream>
#endif 

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
#else
    // TEMP bypass parser until Fran parser is integrated:
    // Build a minimal request and call handler once per connection.
    HttpRequest req;
    req.method = "GET";
    req.uri = "/";            // you can later parse this from _read_buffer
    req.version = "HTTP/1.1";

    // Try to extract Host header very simply (optional for now)
    std::string host = "";
    std::string::size_type pos = _read_buffer.find("Host:");
    if (pos != std::string::npos) {
        pos += 5;
        while (pos < _read_buffer.size() && (_read_buffer[pos] == ' ' || _read_buffer[pos] == '\t'))
            ++pos;
        std::string::size_type end = _read_buffer.find("\r\n", pos);
        if (end != std::string::npos)
            host = _read_buffer.substr(pos, end - pos);
    }

    const RuntimeServer* server = ServerResolver::resolve(_config, _socket_key, host);

    RequestHandler handler;
    HttpResponse res = handler.handle(req, server);
    _write_buffer = res.serialize();
    _state = WRITING;
    return;
#endif
}

    //_state = PARSING;
    // if (_parser.parse(_read_buffer, _request)) {
    //     _state = HANDLING;
    //     _request.print();

    //     RequestHandler handler(_config);
    //     _response = handler.handle(_request);
    //     _write_buffer = _response.toString();
    //     _state = ConnectionState::WRITING;
    //}


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


