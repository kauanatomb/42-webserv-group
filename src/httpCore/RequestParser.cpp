#include "RequestParser.hpp"

/************ Constructor and Destructor*************/ 
RequestParser::RequestParser(void): _state(START_LINE), _error_status(0), _has_error(false)
{}

RequestParser::~RequestParser(void)
{}

/************ Methods ************/
bool RequestParser::parse(std::string& buffer, HttpRequest& request)
{
    /* verify if there is content to parse section*/
    if (_state == START_LINE)
        if (!parseStartLine(buffer, request)) // parseStartLine givrs false for incomplete lines: without CRFL
            return (false);
    _state = HEADERS;
    if (_state == HEADERS)
        if (!parseHeaders)
            return (false)
    //_state = BODY // what is the logic for looking for a body or not
    if (_state == BODY)
        if (!parseBody(buffer, request))
    if (_STATE == COMPLETE)
        return (true);
    //how do you know when body is finished
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

/************ Helper Functions ************/

/* Method SP Request-URI SP HTTP-Version CRLF */
static bool parseStartLine()
{
    /*
    1. check if Startline is complete by findign crlf, if not return false
    2. duplicate line and remove from buffer
    3. store componenets in httprequest class
    4.1 check for components of line : Method(GET/POST/DELETE)
        a. if not correct : change error status and put error code
    4.2 check for components of line : Request-URI(bla/bla)
        a. if not correct : change error status and put error code
    4.3 check for components of line : http VERSION(http1.1 / http1.0)
        a. if not correct : change error status and put error code
    5. checl for CRLF, if not return false
    */
}

/* Header-Field   = Field-Name ":" [Field-Value] CRLF */
static bool parseHeader()
{
    /*
    0. loop to be able to process multiple headers
    1. check for CRLF, if not return false
    2. check if the buffer read is only compose of crlf, if yes remove crlf from buffer, and  change to body and break
    3. store beggining of line and erase from buffer
    4. check for colon inside line(only grammar rule, verify this later), if not throw error
    5. store components in httpRequest class
    0. outside loop: check for mandatory header
    */
}

static bool parseBody()
{
    /*
    Evaluate the state: 
    1.0 check if chunked header exists, verify no content lenght and change state, return true
    1.1 check if body length exists, verify no chunk header, else return false, change error
    1.2 else mark as complete 
        return true
    2. verify content lenght corresponds to buffer size, else throw errro
    3. copy and erase from buffer
    4. assign to httpRequest
    5. change state to complete and return true. 
    */
}

static bool parseChunkSize()
{
    /*
    0. check for CRLF, else return false
    
    */
}