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


////*****Notes */


class RequestParser {
private:
    enum State {
        START_LINE,
        HEADERS,
        BODY,
        COMPLETE,
        ERROR
    };
    
    State _state;
    int _error_status;
    bool _has_error;
    size_t _expected_body_size;
    
public:
    RequestParser() : 
        _state(START_LINE), 
        _error_status(0), 
        _has_error(false),
        _expected_body_size(0) {}
    
    bool parse(std::string& buffer, HttpRequest& request) {
        while (true) {
            
            // ═══════════════════════════════════════════════
            // STATE 1: Parse request line
            // Grammar
            // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
            // ═══════════════════════════════════════════════
            if (_state == START_LINE) {  
                size_t pos = buffer.find("\r\n");  //verify start line is complete by checking CRLN
                if (pos == std::string::npos) {
                    return false;  // Wait for more data
                }
                
                std::string line = buffer.substr(0, pos); //take apart START_LINE content in a copy 
                buffer.erase(0, pos + 2); //clear start line from the buffer
                
                // Parse "GET / HTTP/1.1"
                std::istringstream iss(line);
                iss >> request.method >> request.uri >> request.version; //fill the class parameters 
                                                                         //does this method verify that the space exists
                
                // Validate
                if (!validateMethod(request.method)) { 
                    _error_status = 405; // for this one it just return true without checking the rest of the components if it fails

                    _has_error = true;
                    _state = ERROR;
                    return true;
                } 
                
                if (!validateVersion(request.version)) {
                    _error_status = 505;
                    _has_error = true;
                    _state = ERROR;
                    return true;
                }
                
                // Success, move to next state
                _state = HEADERS;
                continue;
            }
            

            // ═══════════════════════════════════════════════
            // STATE 2: Parse headers
            // Grammar
            // Field-Name ":" [Field-Value] CRLF
            // ═══════════════════════════════════════════════
            if (_state == HEADERS) {

                while (true) {
                    size_t pos = buffer.find("\r\n"); //beginning of check for complet statement 
                    if (pos == std::string::npos) {
                        return false;  // Wait for more data
                    }
                    
                    // Blank line = end of headers
                    if (pos == 0) {
                        buffer.erase(0, 2);
                        _state = BODY;
                        break;
                    }
                    
                    // Parse "Name: Value"
                    std::string line = buffer.substr(0, pos);
                    buffer.erase(0, pos + 2); // end of check of complete statement
                    
                    size_t colon = line.find(':');
                    if (colon == std::string::npos) {
                        _error_status = 400;
                        _has_error = true;
                        _state = ERROR;
                        return true;
                    }
                    
                    std::string name = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);
                    
                    // Store (lowercase name for case-insensitive)
                    request.headers[toLowerCase(name)] = trim(value); // is this a map assigment 
                }
                // add need to check for host as minium heaser 
                continue;
            }
            
            // ═══════════════════════════════════════════════
            // STATE 3: Parse body
            // ═══════════════════════════════════════════════
            if (_state == BODY) {
                // Check for Content-Length
                if (request.headers.count("content-length")) {
                    _expected_body_size = 
                        atoi(request.headers["content-length"].c_str());
                    
                    if (buffer.size() < _expected_body_size) {
                        return false;  // Wait for more data
                    }
                    
                    request.body = buffer.substr(0, _expected_body_size);
                    buffer.erase(0, _expected_body_size);
                }
                // Check for chunked
                else if (request.headers.count("transfer-encoding") &&
                         request.headers["transfer-encoding"] == "chunked") {
                    if (!parseChunked(buffer, request)) {
                        return false;  // Wait for more data
                    }
                }
                // No body
                else {
                    request.body = "";
                }
                
                _state = COMPLETE;
                return true;
            }
            
            // ═══════════════════════════════════════════════
            // STATE 4: Complete or Error
            // ═══════════════════════════════════════════════
            if (_state == COMPLETE || _state == ERROR) {
                return true;
            }
        }
    }
    
    bool getHasError() const { return _has_error; }
    int getErrorStatus() const { return _error_status; }
};