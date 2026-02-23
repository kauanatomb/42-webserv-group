#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP
# include <iostream>
#include "./resolver/RuntimeConfig.hpp"

class RequestParser
{
    private:
        enum State {START_LINE, HEADERS, BODY, COMPLETE };
        State _state;
        int _error_status;
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
        bool isComplete() const;
        bool hasError() const;
        int &getErrorStatus() const;

};

#endif

