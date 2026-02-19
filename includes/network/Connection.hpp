#pragma once

#include "../resolver/RuntimeConfig.hpp"
// #include "../http/RequestParser.hpp"

class Connection {
    public:
        Connection(int fd, const RuntimeConfig& cfg, const SocketKey& listenKey);

        void onReadable();
        void onWritable();

        bool isClosed() const;
        bool wantsWrite() const;

        // int getFd() const;

    private:
        int _socket_fd;
        std::string _read_buffer;
        std::string _write_buffer;

        //listen socket key used to pick candidate servers from RuntimeConfig
        SocketKey _listenKey; //to help ReqeuestHandler know which SocketKey group to use

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
