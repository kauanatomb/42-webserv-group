#include "./httpCore/RequestParser.hpp"
#include <sstream>
#include <stdlib.h> 

/************ Helper Functions ************/
///Utils
static bool checkCompleteLine(std::string& buffer)
{
    size_t pos = buffer.find("\r\n");
    if (pos == std::string::npos) //check if found 
        return (false);
    return (true);
}
static std::string copyAndCleanBuffer(std::string& buffer)
{
    size_t pos = buffer.find("\r\n");
    std::string line = buffer.substr(0, pos);
    buffer.erase(0, pos + 2);
    return (line);
}

///StartLine
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
    if (c == ':' || c == '/' || c ==  '?'|| c == '#' ||c ==  '['|| c ==  ']'
                || c ==  '@'|| c ==  '!' || c ==  '$'|| c ==  '&'|| c ==  '\'' 
                || c == '(' || c ==  ')'|| c == '*'|| c ==  '+' || c == ',' 
                || c ==  ';'|| c == '=')
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
        else if (!(isUnreserved(c) || isReserved(c)))
            return false;
    }
    return true;
}

static bool checkVersion(std::string version)
{
    return (version == "HTTP/1.1" || version == "HTTP/1.0");
}

/* Method SP Request-URI SP HTTP-Version CRLF */
bool RequestParser::parseStartLine(std::string& buffer, HttpRequest& request)
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
bool RequestParser::parseHeader(std::string& buffer, HttpRequest& request)
{
    while (true)
    {
        if (!checkCompleteLine(buffer)) //1
            return (false);
        std::string bufferSection = copyAndCleanBuffer(buffer);
        if (bufferSection == "")
        {
            _state = BODY;
            break;
        }
        size_t posColon = buffer.find(":");
        if (posColon== std::string::npos) //check : 
            return (setErrorInfo(ERROR, 400, true), true); //error unformatted header
        std::string headerName = bufferSection.substr(0, posColon);
        std::string headerValue = bufferSection.substr(posColon, bufferSection.size());
        request.headers[headerName] = headerValue;
    }
    if (request.headers.find("Host") == request.headers.end())
        setErrorInfo(ERROR, 400, true); //Error: missing mandatory header
    return (true);
}

bool RequestParser::parseBody(std::string& buffer, HttpRequest& request)
{
    if (request.headers.find("transfers_encoding") != request.headers.end())
    {
        if (request.headers.find("content_length") == request.headers.end())    
            return (_state = CHUNK_SIZE, true);
        else
            return (setErrorInfo(ERROR, 400, true), true); //invalid header combination
    }
    else if (request.headers.find("content_length") != request.headers.end())
    {
        
        if (request.headers.find("transfers_encoding") != request.headers.end())
            return (setErrorInfo(ERROR, 400, true), true);  //invalid header combination
        else
        {
            size_t contentLen = atoi(request.headers["content_length"].c_str());
            if (buffer.size() < contentLen )
                return (false); //not enough data
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

static bool hexToNum(const std::string& hex, long& result)
{
    if (hex.empty())
        return false;
    char* endptr = NULL;
    errno = 0;
    result = std::strtoul(hex.c_str(), &endptr, 16);
    if (endptr == hex.c_str()) // No digits parsed
        return false;
    if (*endptr != '\0') // Extra invalid characters
        return false;
    if (errno == ERANGE) // Overflow
        return false;
    return true;
}

bool RequestParser::parseChunkSize(std::string& buffer)
{
    if (!checkCompleteLine(buffer))
        return (false);
    std::string sectionBuffer = copyAndCleanBuffer(buffer);
    if (!hexToNum(sectionBuffer, _chunk_size))
        return (setErrorInfo(ERROR, 400, true), true);
    if (_chunk_size == 0)
        _state = FINAL_CRLF;
    else
        _state = CHUNK_DATA;
    return (true);
}

bool RequestParser::parseChunkData(std::string& buffer, HttpRequest& request)
{
    if (!checkCompleteLine(buffer))
        return (false);
    std::string sectionBuffer = copyAndCleanBuffer(buffer);
    if ( _chunk_size != static_cast<long>(sectionBuffer.size()))
        return (setErrorInfo(ERROR, 400, true), true); //bad chunk data
    request.body.append(sectionBuffer);
    _state = CHUNK_SIZE;
    _chunk_size = -1;
    return true;
}

bool RequestParser::parseChunkFinalCRLF(std::string& buffer)
{
    if (!checkCompleteLine(buffer))
        return (false);
    std::string sectionBuffer = copyAndCleanBuffer(buffer);
    if (sectionBuffer != "\r\n")
        return (setErrorInfo(ERROR, 400, true), true); //bad chunk ending
    _state = COMPLETE;
    return true ;
}



/************ Constructor and Destructor*************/ 
RequestParser::RequestParser(void): _state(START_LINE), _error_status(0), _chunk_size(-1), _has_error(false)
{}

RequestParser::~RequestParser(void)
{}

bool RequestParser::parse(std::string& buffer, HttpRequest& request)
{
    std::cout << buffer << std::endl;
    while (true)
    {
        switch (_state)
        {
            case START_LINE:
                if (!parseStartLine(buffer, request))
                    return false;
                _state = HEADERS;
                break;

            case HEADERS:
                if (!parseHeader(buffer, request))
                    return false;
                break;

            case BODY:
                if (!parseBody(buffer, request))
                    return false;
                break;

            case CHUNK_SIZE:
                if (!parseChunkSize(buffer))
                    return false;
                break;

            case CHUNK_DATA:
                if (!parseChunkData(buffer, request))
                    return false;
                break;

            case FINAL_CRLF:
                if (!parseChunkFinalCRLF(buffer))
                    return false;
                break;

            case COMPLETE:
                return true;

            case ERROR:
                return false;
        }
    }
}


bool RequestParser::isComplete() 
{
    if (_state == COMPLETE)
        return (true);
    return (false);
}

bool RequestParser::hasError() 
{
    return (_has_error);
}
int &RequestParser::getErrorStatus() 
{
    return (_error_status);
}


/*************** Getters and setters ***************/
void RequestParser::setErrorInfo(State state, int error_status, bool has_error)
{
    _state = state;
    _error_status = error_status;
    _has_error = has_error;
}