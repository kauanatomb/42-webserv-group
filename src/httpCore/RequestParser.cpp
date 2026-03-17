#include "./httpCore/RequestParser.hpp"
#include <sstream>
#include <stdlib.h>
#include <cerrno>


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

static std::string trimOWS(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
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
    if (!checkCompleteLine(buffer))
        return (false);
    std::string bufferSection = copyAndCleanBuffer(buffer); 
    std::istringstream iss(bufferSection);
    iss >> request.method >> request.uri >> request.version;
    if (!checkURI(request.uri))
        return (setErrorInfo(ERROR, 400, true), true);
    if (!checkVersion(request.version))
        return (setErrorInfo(ERROR, 505, true), true);
    size_t qpos = request.uri.find('?');
    if (qpos != std::string::npos) {
        request.path  = request.uri.substr(0, qpos);
        request.query = request.uri.substr(qpos + 1);
    } else {
        request.path = request.uri;
    }
    // collapse consecutive slashes  (e.g. //foo -> /foo)
    std::string& p = request.path;
    size_t w = 0;
    for (size_t r = 0; r < p.size(); ++r) {
        if (p[r] == '/' && w > 0 && p[w - 1] == '/')
            continue;
        p[w++] = p[r];
    }
    p.resize(w);
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
        if (bufferSection == "") // for empty CRLF
        {
            _state = BODY;
            break;
        }
        size_t posColon = bufferSection.find(":");
        if (posColon== std::string::npos) //check : 
            return (setErrorInfo(ERROR, 400, true), true); //error unformatted header
        std::string headerName = bufferSection.substr(0, posColon);
        std::string headerValue = bufferSection.substr(posColon + 1);
        request.headers[trimOWS(headerName)] = trimOWS(headerValue);
    }
    if (request.headers.find("Host") == request.headers.end())
        setErrorInfo(ERROR, 400, true); //Error: missing mandatory header
    return (true);
}

bool RequestParser::parseBody(std::string& buffer, HttpRequest& request)
{
    bool hasTE = request.headers.find("Transfer-Encoding") != request.headers.end();
    bool hasCL = request.headers.find("Content-Length") != request.headers.end();

    if (hasTE && hasCL)
        return (setErrorInfo(ERROR, 400, true), true); //invalid header combination

    if (hasTE)
    {
        if (request.headers["Transfer-Encoding"] != "chunked")
            return (setErrorInfo(ERROR, 501, true), true); //only chunked is supported
        return (_state = CHUNK_SIZE, true);
    }

    if (hasCL)
    {
        const std::string& clValue = request.headers["Content-Length"];
        char* endptr = NULL;
        errno = 0;
        unsigned long contentLen = std::strtoul(clValue.c_str(), &endptr, 10);
        if (endptr == clValue.c_str() || *endptr != '\0' || errno == ERANGE)
            return (setErrorInfo(ERROR, 400, true), true); //invalid Content-Length
        if (buffer.size() < contentLen)
            return (false); //not enough data
        request.body.append(buffer, 0, contentLen);
        buffer.erase(0, contentLen);
        _state = COMPLETE;
        return (true);
    }

    _state = COMPLETE;
    return (true);
}

bool RequestParser::parseChunkSize(std::string& buffer)
{
    if (!checkCompleteLine(buffer))
        return false;
    std::string line = copyAndCleanBuffer(buffer);
    // Extract hex part before ';'
    size_t semi = line.find(';');
    std::string hexPart = (semi == std::string::npos)
        ? line
        : line.substr(0, semi);
    hexPart = trimOWS(hexPart);
    if (hexPart.empty())
        return (setErrorInfo(ERROR, 400, true), true);
    char* endptr = NULL;
    errno = 0;
    unsigned long size = std::strtoul(hexPart.c_str(), &endptr, 16);
    if (*endptr != '\0' || errno == ERANGE)
        return (setErrorInfo(ERROR, 400, true), true);
    _chunk_size = size;
    _state = (_chunk_size == 0) ? FINAL_CRLF : CHUNK_DATA;
    return true;
}

bool RequestParser::parseChunkData(std::string& buffer, HttpRequest& request)
{
    // Need exactly _chunk_size bytes + trailing CRLF
    if (static_cast<long>(buffer.size()) < _chunk_size + 2)
        return (false);
    if (buffer[_chunk_size] != '\r' || buffer[_chunk_size + 1] != '\n')
        return (setErrorInfo(ERROR, 400, true), true); //bad chunk data: missing trailing CRLF
    request.body.append(buffer, 0, _chunk_size);
    buffer.erase(0, _chunk_size + 2);
    _state = CHUNK_SIZE;
    _chunk_size = -1;
    return true;
}

bool RequestParser::parseChunkFinalCRLF(std::string& buffer)
{
    if (!checkCompleteLine(buffer))
        return (false);
    std::string sectionBuffer = copyAndCleanBuffer(buffer);
    if (!sectionBuffer.empty())
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
                return true;
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
