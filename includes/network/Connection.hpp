#pragma once

#include "../resolver/RuntimeConfig.hpp"
// #include "../http/RequestParser.hpp"
// #include "../http/HttpResponse.hpp"

class Connection {
    public:
        Connection(int fd, const RuntimeConfig& cfg, const SocketKey& socket_key);

        void onReadable();
        void onWritable();

        bool isClosed() const;
        bool wantsWrite() const;

    private:
        int _socket_fd;
        SocketKey _socket_key;
        std::string _read_buffer;
        std::string _write_buffer;

        // RequestParser _parser;
        // HttpRequest _request;
        // HttpResponse _response;

        const RuntimeConfig& _config;

        enum ConnectionState { 
            READING, // waiting for client data 
            PARSING, // parse in process 
            HANDLING, // RequestHandler active 
            WRITING, // sending response 
            CLOSED // connection ready to be closed 
        };

        ConnectionState _state;
        bool _keep_alive;
};
