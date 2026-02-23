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
        parseStartLine()
}

bool RequestParser::isComplete() const
{}

bool RequestParser::hasError() const
{

}
int &RequestParser::getErrorStatus() const
{
    return (_error_status);
}


/************ Utils ************/
static bool false checkCRLF()
{

        return (status);
}
/************ Helper Functions ************/

/* Request-Line = Method SP Request-URI SP HTTP-Version CRLF*/
static bool  parseStartLine()
{
    /* 0. validate CRLF */
            size_t pos = buffer.find("\r\n");
        if (pos == std::string::npos) {
            return false;  // Need more data
        }
    /* 1. check method */
    /* 2. check space */
    /* 3. check URI */
    /* 4. check space */
    /* 2.  */
}
