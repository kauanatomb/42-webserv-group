#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP
# include <iostream>
#include "./httpCore/HttpRequest.hpp"

class RequestParser
{
    private:
        enum State {START_LINE, HEADERS, BODY, CHUNK_SIZE, CHUNK_DATA, FINAL_CRLF, COMPLETE, ERROR };
        State _state;
        int _error_status;
        long _chunk_size;
        bool _has_error;

    public:
        /*************** Class defaults ***************/
        RequestParser(void);
        //RequestParser(const RequestParser& other);
        //RequestParser &operator=(const RequestParser &other);
        ~RequestParser();
        /*************** Methods ***************/
        /** 
            @brief Return false if incomplete, true if complete or error 
        */
        bool parse(std::string& buffer, HttpRequest& request);

        bool parseStartLine(std::string& buffer, HttpRequest& request);

        bool parseHeader(std::string& buffer, HttpRequest& request);
        bool parseBody(std::string& buffer, HttpRequest& request);
        bool parseChunkSize(std::string& buffer);
        bool parseChunkData(std::string& buffer, HttpRequest& request);
        bool parseChunkFinalCRLF(std::string& buffer);

        bool isComplete();
        bool hasError();
        int &getErrorStatus();
        
        /*************** Getters and Setters ***************/
        void setErrorInfo(State state, int error_status, bool has_error);

};

#endif

