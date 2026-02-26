#include "RequestParser.hpp"

/************ Constructor and Destructor*************/ 
RequestParser::RequestParser(void): _state(START_LINE), _error_status(0), _has_error(false)
{}

RequestParser::~RequestParser(void)
{}

/************ Methods ************/
bool RequestParser::parse(std::string& buffer, HttpRequest& request)
{
    /* verify if there is content to parse section - not necessary for the moment*/
    if (_state == START_LINE)
        if (!parseStartLine(buffer, request)) // parseStartLine givrs false for incomplete lines: without CRFL
            return (false);
    _state = HEADERS;
    if (_state == HEADERS)
        if (!parseHeaders)
            return (false)
    if (_state == BODY)
        if (!parseBody(buffer, request))
    if (_state = )
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


static bool checkCompleteLine(std::string& buffer)
{
    size_t pos = buffer.find("\r\n");
    if (pos == std::string::npos) //check if found 
        return (false);
    return (true);
}
static std::string copyAndCleanBuffer(std::string& buffer, int pos)
{
    size_t pos = buffer.find("\r\n");
    std::string line = buffer.substr(0, pos);
    buffer.erase(0, pos + 2);
}


static bool checkMethod(std::string method)
{
    return (method == "GET" ||method == "POST" || method == "DELETE");
}

static bool isHex(char c)
{
    return std::isxdigit(static_cast<unsigned char>(c)) != 0;
}

static bool isUnreserved(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) ||
           c == '-' || c == '.' || c == '_' || c == '~';
}

static bool isReserved(char c)
{
    if (c = ':' || c = '/' || c =  '?'|| c = '#' ||c =  '['|| c =  ']'
                || c =  '@'|| c =  '!' || c =  '$'|| c =  '&'|| c =  '\'' 
                || c = '(' || c =  ')'|| c = '*'|| c =  '+' || c = ',' 
                || c =  ';'|| c = '=')
        return true;
    return false;
}


static bool checkURI(const std::string& uri)
{

    if (uri.empty() || uri[0] != '/')
        return false;

    for (size_t i = 0; i < uri.size(); ++i)
    {
        unsigned char c = uri[i];

        if (c < 32 || c == 127)
            return false;

        if (c == '%')
        {
            if (i + 2 >= uri.size())
                return false;
            if (!isHex(uri[i + 1]) || !isHex(uri[i + 2]))
                return false;
            i += 2; 
        }
        else if (!(sUnreserved(c) || isReserved(c)))
            return false;
    }
    return true;
}

static bool checkVersion(std::string version)
{
    return (version = "HTTP/1.1" || version = "HTTP/1.0");
}


/*
1. check if Startline is complete by findign crlf, if not return false D
2. duplicate line and remove from buffer D 
3. store componenets in httprequest class 
4.1 check for components of line : Method(GET/POST/DELETE)
    a. if not correct : change error status and put error code
4.2 check for components of line : Request-URI(bla/bla)
    a. if not correct : change error status and put error code
4.3 check for components of line : http VERSION(http1.1 / http1.0)
    a. if not correct : change error status and put error code
*/

/* Method SP Request-URI SP HTTP-Version CRLF */
static bool parseStartLine(std::string& buffer, HttpRequest& request)
{
    if (!checkCompleteLine(buffer)) //1
        return (false);
    std::string bufferSection = copyAndCleanBuffer(buffer); //2
    std::istringstream iss(bufferSection);
    iss >> request.method >> request.uri >> request.version; //3
    if (!checkMethod(request.method))
        return (setErrorInfo(ERROR, 405, true), true); 
    if (!checkURI(request.uri ))
        return (setErrorInfo(ERROR, 400, true), true);
    if (!checkVersion(request.version))
        return (setErrorInfo(ERROR, 505, true), true);
    return (true);
}

/* Header-Field   = Field-Name ":" [Field-Value] CRLF */
static bool parseHeader(std::string& buffer, HttpRequest& request)
{
    while (true)
    {
        if (!checkCompleteLine(buffer)) //1
            return (false);
        std::string bufferSection = copyAndCleanBuffer(buffer);
        if (bufferSection == "")
        {
            state = BODY
            break;
        }
        size_t posColon = buffer.find(":");
        if (posColon== std::string::npos) //check : 
            return (setErrorInfo(ERROR, 400, true), true); //error unformatted header
        std::string headerName = bufferSection.substr(0, posColon);
        std::string headerValue = bufferSection.substr(posColon, bufferSection.size());
        headers[headerName] = headerValue;
    }
    if (headers.find("Host") == headers.end())
        setErrorInfo(ERROR, 400, true); //Error: missing mandatory header
    return (true);
    /*
    0. loop to be able to process multiple headers D
    1. check for CRLF, if not return false D
    2. store beggining of line and erase from buffer D
    3. check if the buffer read is only compose of crlf, if yes remove crlf from buffer, and  change to body and break D
    4. check for colon inside line(only grammar rule, verify this later), if not throw error
    5. store components in httpRequest class
    0. outside loop: check for mandatory header
    */
}


/*
Evaluate the state: 
1.0 check if chunked header exists, verify no content length and change state-CHUNK_SIZE, return true
1.1 check if body length exists, verify no chunk header, else return false, change error
1.2 else mark as complete 
    return true
2. verify content lenght corresponds to buffer size, else throw errro
3. copy and erase from buffer
4. assign to httpRequest
5. change state to complete and return true. 
*/

static bool parseBody(std::string& buffer, HttpRequest& request)
{
    if (headers.find("transfers_encoding") != request.headers.end())
    {
        if (headers.find("content_length") == requestheaders.end())    
            return (_state = CHUNK_SIZE, true);
        else
            return (setErrorInfo(ERROR, 400, true), true); //invalid header combination
    }
    else if (headers.find("content_length") != request.headers.end())
    {
        
        if (headers.find("transfers_encoding") != request.headers.end())
            return (setErrorInfo(ERROR, 400, true), true);  //invalid header combination
        else
        {
            size_t contentLen = atoi(request.headers["content_length"].c_str());
            if (buffer.size() < contentLen )
                return (false) //not enough data
            std::string body = buffer.substr(0, contentLen);
            buffer.erase(0, contentLen);
            request.body.append(body);
            _state = COMPLETE;
            return (true);
        }
    }
    else 
        return (_state = COMPLETE, true);


}


/*
Chunked-Body =
    (size in hex)
    \r\n
    (exactly that many bytes)
    \r\n
    repeat...
    until size == 0
    then:
        0\r\n
        \r\n
*/
static bool parseChunkSize()
{

}


    /*
    State = CHUNK_SIZE

loop:

    if State == CHUNK_SIZE:
        if no complete line in buffer:
            return false

        read line until CRLF
        parse hex number → chunk_size

        if invalid hex:
            set ERROR
            return false

        remove line + CRLF from buffer

        if chunk_size == 0:
            State = FINAL_CRLF
        else:
            State = CHUNK_DATA


    if State == CHUNK_DATA:
        if buffer.size < chunk_size:
            return false

        read exactly chunk_size bytes
        append to request.body
        remove them from buffer

        State = CHUNK_CRLF


    if State == CHUNK_CRLF:
        if buffer.size < 2:
            return false

        if next two bytes != "\r\n":
            set ERROR
            return false

        remove "\r\n"
        State = CHUNK_SIZE


    if State == FINAL_CRLF:
        if buffer.size < 2:
            return false

        if next two bytes != "\r\n":
            set ERROR
            return false

        remove "\r\n"
        State = COMPLETE
        return true
    */
/*************** Getters and setters ***************/
void RequestParser::setErrorInfo(State state, int error_status, bool has_error)
{
    _state = state;
    error_status = _error_status;
    has_error = _has_error;
}