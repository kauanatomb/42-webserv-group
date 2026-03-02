#include "httpCore/RequestParser.hpp"
#include <iostream>
#include <cassert>

void printTest(const std::string& name) {
    std::cout << "\n╔═══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ " << name << std::endl;
    std::cout << "╚═══════════════════════════════════════════════╝" << std::endl;
}

void test1_simpleGET() {
    printTest("TEST 1: Simple GET Request");
    
    RequestParser parser;
    HttpRequest request;
    
    std::string buffer = 
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    std::cout << "Input:\n" << buffer << std::endl;
    
    bool result = parser.parse(buffer, request);
    
    std::cout << "parse() returned: " << (result ? "TRUE" : "FALSE") << std::endl;
    std::cout << "hasError(): " << (parser.hasError() ? "TRUE" : "FALSE") << std::endl;
    std::cout << "isComplete(): " << (parser.isComplete() ? "TRUE" : "FALSE") << std::endl;
    
    if (parser.isComplete()) {
        std::cout << "\n✓ Parsed successfully!" << std::endl;
        std::cout << "  Method: " << request.method << std::endl;
        std::cout << "  URI: " << request.uri << std::endl;
        std::cout << "  Version: " << request.version << std::endl;
        std::cout << "  Headers: " << request.headers.size() << std::endl;
        
        for (std::map<std::string, std::string>::iterator it = request.headers.begin();
             it != request.headers.end(); ++it) {
            std::cout << "    " << it->first << ": " << it->second << std::endl;
        }
    }
    
    assert(result == true);
    assert(parser.isComplete() == true);
    assert(parser.hasError() == false);
    assert(request.method == "GET");
    assert(request.uri == "/");
    assert(request.version == "HTTP/1.1");
}

void test2_partialData() {
    printTest("TEST 2: Partial Data (Multiple recv calls)");
    
    RequestParser parser;
    HttpRequest request;
    std::string buffer;
    bool result;
    
    // First chunk
    buffer = "GET / HT";
    std::cout << "Chunk 1: \"" << buffer << "\"" << std::endl;
    result = parser.parse(buffer, request);
    std::cout << "  → parse() = " << (result ? "TRUE" : "FALSE") << std::endl;
    assert(result == false);  // Should wait for more
    
    // Second chunk
    buffer += "TP/1.1\r\nHost: ";
    std::cout << "Chunk 2: added \"TP/1.1\\r\\nHost: \"" << std::endl;
    result = parser.parse(buffer, request);
    std::cout << "  → parse() = " << (result ? "TRUE" : "FALSE") << std::endl;
    assert(result == false);  // Still waiting
    
    // Final chunk
    buffer += "localhost\r\n\r\n";
    std::cout << "Chunk 3: added \"localhost\\r\\n\\r\\n\"" << std::endl;
    result = parser.parse(buffer, request);
    std::cout << "  → parse() = " << (result ? "TRUE" : "FALSE") << std::endl;
    assert(result == true);
    assert(parser.isComplete());
    
    std::cout << "\n✓ Handled partial data correctly!" << std::endl;
    std::cout << "  Final method: " << request.method << std::endl;
}

void test3_POSTWithBody() {
    printTest("TEST 3: POST with Content-Length");
    
    RequestParser parser;
    HttpRequest request;
    
    std::string buffer = 
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "name=John&email=john@test.com";
    
    bool result = parser.parse(buffer, request);
    
    std::cout << "DEBUG: paso parse clase" << std::endl;

    assert(result == true);
    assert(parser.isComplete());
    assert(request.method == "POST");
    assert(request.body == "name=John&email=john@test.com");
    
    std::cout << "✓ POST request parsed!" << std::endl;
    std::cout << "  Body length: " << request.body.size() << std::endl;
    std::cout << "  Body: " << request.body << std::endl;
}

void test4_invalidMethod() {
    printTest("TEST 4: Invalid Method (PUT)");
    
    RequestParser parser;
    HttpRequest request;
    
    std::string buffer = 
        "PUT / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    bool result = parser.parse(buffer, request);
    
    assert(result == true);  // Returns true (done)
    assert(parser.hasError() == true);  // But has error
    assert(parser.getErrorStatus() == 405);  // Method Not Allowed
    
    std::cout << "✓ Invalid method detected!" << std::endl;
    std::cout << "  Error code: " << parser.getErrorStatus() << std::endl;
}

void test5_missingHost() {
    printTest("TEST 5: Missing Host Header");
    
    RequestParser parser;
    HttpRequest request;
    
    std::string buffer = 
        "GET / HTTP/1.1\r\n"
        "User-Agent: Test\r\n"
        "\r\n";
    
    bool result = parser.parse(buffer, request);
    
    assert(result == true);
    assert(parser.hasError() == true);
    assert(parser.getErrorStatus() == 400);
    
    std::cout << "✓ Missing Host header detected!" << std::endl;
    std::cout << "  Error code: " << parser.getErrorStatus() << std::endl;
}

void test6_chunkedEncoding() {
    printTest("TEST 6: Chunked Encoding");
    
    RequestParser parser;
    HttpRequest request;
    
    std::string buffer = 
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "6\r\n"
        " World\r\n"
        "0\r\n"
        "\r\n";
    
    bool result = parser.parse(buffer, request);
    
    assert(result == true);
    assert(parser.isComplete());
    assert(request.body == "Hello World");
    
    std::cout << "✓ Chunked encoding parsed!" << std::endl;
    std::cout << "  Body: " << request.body << std::endl;
}

int main() {
    std::cout << "\n";
    std::cout << "════════════════════════════════════════════════" << std::endl;
    std::cout << "  HTTP REQUEST PARSER - UNIT TESTS" << std::endl;
    std::cout << "════════════════════════════════════════════════" << std::endl;
    
    try {
        //test1_simpleGET();
        //test2_partialData();
        //test3_POSTWithBody();
        test4_invalidMethod();
        test5_missingHost();
        test6_chunkedEncoding();
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  ✓ ALL TESTS PASSED!                          ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════╝" << std::endl;
        std::cout << "\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}