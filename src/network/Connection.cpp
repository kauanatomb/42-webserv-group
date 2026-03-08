#include "network/Connection.hpp"
#include "resolver/HandlerResolver.hpp"
#include "httpCore/ErrorHandler.hpp"
#include <sys/socket.h>
# include "httpCore/RequestHandler.hpp"
# include <string>

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

    //if (_parser.parse(_read_buffer, _request)) {
    //if request is not complete yet, do NOT respond

    if (!_parser.parse(_read_buffer, _request)) {

        _state = READING;
        return;
    }

    //request is complete or parser error
    if (_parser.hasError()) {
        int status = _parser.getErrorStatus();
            //(void)status; TODO remove when HttpResponse::fromStatus(status) will be ready
        // _response = HttpResponse::fromStatus(status);
        _response = ErrorHandler::build(status, (const RuntimeLocation*)NULL);
        } else {
            _request.print();
            const RuntimeLocation* loc = HandlerResolver::resolve(_config, _socket_key, _request);
            if (!loc) {
                _response = ErrorHandler::build(500, "Resolver returned NULL location \n", (const RuntimeLocation*)NULL);
            } else {
                RequestHandler handler;
                _response = handler.handle(_request, loc);
            }
        }
    
    _write_buffer = _response.serialize();
    _state = WRITING;
}

void Connection::onWritable() {
    ssize_t bytes = send(_socket_fd, _write_buffer.c_str(), _write_buffer.size(), 0);
    if (bytes <= 0) {
        _state = CLOSED;
        return;
    }

    _write_buffer.erase(0, bytes);

    if (_write_buffer.empty()) {
        // if (_keep_alive) resetForNextRequest();
        // else
        _state = CLOSED;
    }
}