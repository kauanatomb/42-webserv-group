#pragma once

#include "../resolver/RuntimeConfig.hpp"
#include "../httpCore/RequestParser.hpp"
#include "../httpCore/HttpResponse.hpp"

struct CgiState {
    pid_t       pid;
    int         stdin_fd;
    int         stdout_fd;
    std::string input;
    size_t      input_offset;
    std::string output;
    bool        done;
    int         error;      // 0 = ok, or code HTTP of error

    CgiState() : pid(-1), stdin_fd(-1), stdout_fd(-1),
                input_offset(0), done(false), error(0) {}

    bool active() const { return pid != -1; }
};

class Connection {
    public:
        Connection(int fd, const RuntimeConfig& cfg, const SocketKey& socket_key);

        void onReadable();
        void onWritable();

        bool isClosed() const;
        bool wantsWrite() const;
        void markActivity();
        //cgi
        int  getCgiStdoutFd() const;
        bool hasCgi()         const;
        int  getCgiStdinFd()  const;
        void onCgiWritable();
        void onCgiReadable();
        void finalizeCgi();
        int  isCgiDone()  const;
        bool isIdleSince(time_t now, int seconds) const;
        time_t getCgiStartTime() const;
        void handleCgiTimeout();

    private:
        int _socket_fd;
        SocketKey _socket_key;
        std::string _read_buffer;
        std::string _write_buffer;
        CgiState _cgi;

        RequestParser _parser;
        HttpRequest _request;
        HttpResponse _response;
        const RuntimeLocation* _active_loc;

        const RuntimeConfig& _config;

        enum ConnectionState { 
            READING, // waiting for client data 
            PARSING, // parse in process 
            HANDLING, // RequestHandler active 
            WRITING, // sending response 
            CGI_WRITING,   // sending body to CGI by stdin pipe
            CGI_READING,   // reading stdout of CGI
            CLOSED // connection ready to be closed 
        };

        ConnectionState _state;
        time_t _last_activity;
        time_t _cgi_start_time;

        void processRequest();
};
