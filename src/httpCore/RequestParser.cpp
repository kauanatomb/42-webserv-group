#include "RequestParser.hpp"

/************ Constructor and Destructor*************/ 
RequestParser::RequestParser(void): _state(START_LINE), _error_status(0), _has_error(false)
{}

RequestParser::~RequestParser(void)
{}

/************ Other Methods ************/
bool RequestParser::parse(std::string& buffer, HttpRequest& request)
{
    if (_state == START_LINE)
        if (!parseStartLine(buffer, request))
            return (false);
    _state = HEADERS;

}

bool RequestParser::isComplete() const
{
    if (_state == COMPLETE)
        return (true);
    return (false;)
}

bool RequestParser::hasError() const
{
    return (_has_error);
}
int &RequestParser::getErrorStatus() const
{
    return (_error_status);
}


/************ Utils ************/
static bool false checkCRLF()
{
        
}
/************ Helper Functions ************/


static bool validateMethod(std::string methodName)
{
    if (methodName == "GET" || methodName == "POST" || methodName == "DELETE")
        return (true);
    return (false);
}


static bool validateVersion(std::string versionName)
{
    //logic to verify:  HTTP 1.0 or 1.1
}
/* 
Grammar rule: 
    Request-Line = Method SP Request-URI SP HTTP-Version CRLF
*/
static bool  parseStartLine(std::string& buffer, HttpRequest& request)
{
    /* 0. validate CRLF */
    size_t pos = buffer.find("\r\n");
    if (pos == std::string::npos) {
        return false;  // Need more data
    }
    
    std::string line = buffer.substr(0, pos);
    std::istringstream iss(line);
    iss >> request.method >> request.uri >> request.version;
    /* 1. check method */
    if (!validateMethod(request.method)) {
        _error_status = 405;
        _has_error = true;
        _state = ERROR;
        return true;
    }
    /* 2. check space */

    /* 3. check URI */
    /* 4. check space */
    /* 2.  */
}
