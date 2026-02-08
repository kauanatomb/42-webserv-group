#include "config/ConfigParser.hpp"
#include "debug/ASTPrinter.hpp"
#include "debug/TokenBuilder.hpp"
#include <exception>

#define RESET   "\033[0m"
#define PINK    "\033[35m"
#define RED     "\033[31m"


void runTest(const std::string& testName, const std::vector<Token>& tokens)
{
    std::cout << PINK << "==============================" << RESET << std::endl;
    std::cout << "Running " << testName << std::endl;
    std::cout << PINK << "==============================" << RESET << std::endl;

    try
    {
        ConfigParser parser(tokens);
        ConfigAST config = parser.parse();
        ASTPrinter::print(config);
        std::cout << "✅ " << testName << " parsed successfully\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ " << testName << " failed: " << std::endl << e.what() << std::endl;
    }

    std::cout << std::endl << std::endl;
}


int main()
{
    //Token for testing(aggregate initialization so class Tokens is not called )
    
    /* Test1 : Minimal valid config
        
        server {
            listen 8080;
        }
        Expected Behaviour: pass ✅
    */

    std::vector<Token> test1;
    test1.push_back(TokenBuilder::makeWord("server"));
    test1.push_back(TokenBuilder::makeLBrace());
    test1.push_back(TokenBuilder::makeWord("listen"));
    test1.push_back(TokenBuilder::makeWord("8080"));
    test1.push_back(TokenBuilder::makeSemicolon());
    test1.push_back(TokenBuilder::makeRBrace());
    
    /* Test 2: Multiple arguments

        server {
            listen 8080;
            error_page 404 500 502 /errors.html;
            location bla {
                allowed_methods GET POST PUT DELETE;
                index index.html index.htm default.html;
            }
        }
        Expected Behaviour: pass ✅
        Current result: ok
    */

    std::vector<Token> test2;
    test2.push_back(TokenBuilder::makeWord("server"));
    test2.push_back(TokenBuilder::makeLBrace());

    test2.push_back(TokenBuilder::makeWord("listen"));
    test2.push_back(TokenBuilder::makeWord("8080"));
    test2.push_back(TokenBuilder::makeSemicolon());

    test2.push_back(TokenBuilder::makeWord("error_page"));
    test2.push_back(TokenBuilder::makeWord("404"));
    test2.push_back(TokenBuilder::makeWord("500"));
    test2.push_back(TokenBuilder::makeWord("502"));
    test2.push_back(TokenBuilder::makeWord("/errors.html"));
    test2.push_back(TokenBuilder::makeSemicolon());

    test2.push_back(TokenBuilder::makeWord("location"));
    test2.push_back(TokenBuilder::makeWord("bla"));
    test2.push_back(TokenBuilder::makeLBrace());

    test2.push_back(TokenBuilder::makeWord("allowed_methods"));
    test2.push_back(TokenBuilder::makeWord("GET"));
    test2.push_back(TokenBuilder::makeWord("POST"));
    test2.push_back(TokenBuilder::makeWord("PUT"));
    test2.push_back(TokenBuilder::makeWord("DELETE"));
    test2.push_back(TokenBuilder::makeSemicolon());

    test2.push_back(TokenBuilder::makeWord("index"));
    test2.push_back(TokenBuilder::makeWord("index.html"));
    test2.push_back(TokenBuilder::makeWord("index.htm"));
    test2.push_back(TokenBuilder::makeWord("default.html"));
    test2.push_back(TokenBuilder::makeSemicolon());

    test2.push_back(TokenBuilder::makeRBrace());
    test2.push_back(TokenBuilder::makeRBrace());

    /* Test 3: Missing Semicolon (Directive: WORD WORD* ";")
        server {
            listen 8080  
            root /var/www;
        }
        Expected Behaviour: pass ✅
        Current result: ok
    */

    std::vector<Token> test3;
    test3.push_back(TokenBuilder::makeWord("server"));
    test3.push_back(TokenBuilder::makeLBrace());

    test3.push_back(TokenBuilder::makeWord("listen"));
    test3.push_back(TokenBuilder::makeWord("8080")); // ❌ no semicolon

    test3.push_back(TokenBuilder::makeWord("root"));
    test3.push_back(TokenBuilder::makeWord("/var/www"));
    test3.push_back(TokenBuilder::makeSemicolon());

    test3.push_back(TokenBuilder::makeRBrace());



    /* Test 4: Directive with only one word  (Directive: WORD WORD* ";")
        server {
            listen ;
        }
        Expected Behaviour: pass ✅
        Current result: ok
    */
    
    std::vector<Token> test4;
    test4.push_back(TokenBuilder::makeWord("server"));
    test4.push_back(TokenBuilder::makeLBrace());

    test4.push_back(TokenBuilder::makeWord("listen"));
    test4.push_back(TokenBuilder::makeSemicolon()); // ❌ missing argument

    test4.push_back(TokenBuilder::makeRBrace());




    /* Test 5: Missing Opening Brace (Server: "server" "{" (directive | location)* "}")
        server
            listen 8080;
        }
        Expected Behaviour: throw error ❌
        Current result: ok 
    */
    
    std::vector<Token> test5;
    test5.push_back(TokenBuilder::makeWord("server"));
    test5.push_back(TokenBuilder::makeWord("listen"));
    test5.push_back(TokenBuilder::makeWord("8080"));
    test5.push_back(TokenBuilder::makeSemicolon());

    test5.push_back(TokenBuilder::makeRBrace()); // ❌ unexpected


    /* Test 6: Missing Closing Brace (Server: "server" "{" (directive | location)* "}")
        server {
            listen 8080;

        Expected Behaviour: throw error ❌ 
        Current result: ok
    */

    std::vector<Token> test6;
    test6.push_back(TokenBuilder::makeWord("server"));
    test6.push_back(TokenBuilder::makeLBrace());

    test6.push_back(TokenBuilder::makeWord("listen"));
    test6.push_back(TokenBuilder::makeWord("8080"));
    test6.push_back(TokenBuilder::makeSemicolon());
    // ❌ no RBRACE


    /* Test 7: Location outside of server (Server: "server" "{" (directive | location)* "}")

        location / {
            index index.html;
        }
        server 
        { 
        }
        Expected Behaviour: throw error ❌
        Current result: ok 
    */
    std::vector<Token> test7;
    test7.push_back(TokenBuilder::makeWord("location"));
    test7.push_back(TokenBuilder::makeWord("/"));
    test7.push_back(TokenBuilder::makeLBrace());

    test7.push_back(TokenBuilder::makeWord("index"));
    test7.push_back(TokenBuilder::makeWord("index.html"));
    test7.push_back(TokenBuilder::makeSemicolon());
    test7.push_back(TokenBuilder::makeRBrace());

    test7.push_back(TokenBuilder::makeWord("server"));
    test7.push_back(TokenBuilder::makeLBrace());
    test7.push_back(TokenBuilder::makeRBrace());



    /* Test 8: empty config
    */
    std::vector<Token> test8;

    /* Test 9: directive outside server (Server: "server" "{" (directive | location)* "}")
        listen 8080;
        Expected Behaviour: throw error ❌ 
        Current result: ok, but revise with team, or see subject
    */
    
    std::vector<Token> test9;
    test9.push_back(TokenBuilder::makeWord("listen"));
    test9.push_back(TokenBuilder::makeWord("8080"));
    test9.push_back(TokenBuilder::makeSemicolon());


    /* Test 10: Extra Closing Brace (Server: "server" "{" (directive | location)* "}")
        server {
            listen 8080;
        }
        }
        Expected Behaviour: throw error ❌ 
        Current result: ok
    */
    
    std::vector<Token> test10;
    test10.push_back(TokenBuilder::makeWord("server"));
    test10.push_back(TokenBuilder::makeLBrace());

    test10.push_back(TokenBuilder::makeWord("listen"));
    test10.push_back(TokenBuilder::makeWord("8080"));
    test10.push_back(TokenBuilder::makeSemicolon());

    test10.push_back(TokenBuilder::makeRBrace());
    test10.push_back(TokenBuilder::makeRBrace()); // ❌ extra


    /* Test 11: invalid keyword (Server: "server" "{" (directive | location)* "}")
        website {
            listen 8080;
        }
        Expected Behaviour: throw error ❌ 
        Current result:ok, but check error message
    */
    std::vector<Token> test11;
    test11.push_back(TokenBuilder::makeWord("website")); // ❌ invalid
    test11.push_back(TokenBuilder::makeLBrace());

    test11.push_back(TokenBuilder::makeWord("listen"));
    test11.push_back(TokenBuilder::makeWord("8080"));
    test11.push_back(TokenBuilder::makeSemicolon());

    test11.push_back(TokenBuilder::makeRBrace());


    /* Test 12: Location without path (Location: "location" WORD "{" directive* "}")
        server {
            listen 8080;
            location {
                index index.html;
            }
        }
        Expected Behaviour: throw error ❌
        Current result: ok, but check message  
    */
    std::vector<Token> test12;
    test12.push_back(TokenBuilder::makeWord("server"));
    test12.push_back(TokenBuilder::makeLBrace());

    test12.push_back(TokenBuilder::makeWord("listen"));
    test12.push_back(TokenBuilder::makeWord("8080"));
    test12.push_back(TokenBuilder::makeSemicolon());

    test12.push_back(TokenBuilder::makeWord("location")); // ❌ no path
    test12.push_back(TokenBuilder::makeLBrace());

    test12.push_back(TokenBuilder::makeWord("index"));
    test12.push_back(TokenBuilder::makeWord("index.html"));
    test12.push_back(TokenBuilder::makeSemicolon());

    test12.push_back(TokenBuilder::makeRBrace());
    test12.push_back(TokenBuilder::makeRBrace());



    /* Test 13: nested location  (Location: "location" WORD "{" directive* "}")
        server {
            location / {
                location /nested {
                    index index.html;
                }
            }
        }
        Expected Behaviour: throw error ❌ 
        Current result: ok, but check message location 
    */

    std::vector<Token> test13;
    test13.push_back(TokenBuilder::makeWord("server"));
    test13.push_back(TokenBuilder::makeLBrace());

    test13.push_back(TokenBuilder::makeWord("location"));
    test13.push_back(TokenBuilder::makeWord("/"));
    test13.push_back(TokenBuilder::makeLBrace());

    test13.push_back(TokenBuilder::makeWord("location"));
    test13.push_back(TokenBuilder::makeWord("/nested"));
    test13.push_back(TokenBuilder::makeLBrace());

    test13.push_back(TokenBuilder::makeWord("index"));
    test13.push_back(TokenBuilder::makeWord("index.html"));
    test13.push_back(TokenBuilder::makeSemicolon());

    test13.push_back(TokenBuilder::makeRBrace());
    test13.push_back(TokenBuilder::makeRBrace());
    test13.push_back(TokenBuilder::makeRBrace());


    /* Test 14: Location  without directive (Location: "location" WORD "{" directive* "}")
        server {
            location / {
            }
        }
        Expected Behaviour: pass ✅
        Current result: wrong check logic 
    */

    std::vector<Token> test14;
    test14.push_back(TokenBuilder::makeWord("server"));
    test14.push_back(TokenBuilder::makeLBrace());

    test14.push_back(TokenBuilder::makeWord("location"));
    test14.push_back(TokenBuilder::makeWord("/"));
    test14.push_back(TokenBuilder::makeLBrace());

    test14.push_back(TokenBuilder::makeRBrace());
    test14.push_back(TokenBuilder::makeRBrace());


    /* Test 15: Location  without closing brace (Location: "location" WORD "{" directive* "}")
        server {
            location / {
        }
        Expected Behaviour: throw error ❌ 
        Current result: revise the next element passing logic 
    */


    std::vector<Token> test15;
    test15.push_back(TokenBuilder::makeWord("server"));
    test15.push_back(TokenBuilder::makeLBrace());
    
    test15.push_back(TokenBuilder::makeWord("location"));
    test15.push_back(TokenBuilder::makeWord("/"));
    test15.push_back(TokenBuilder::makeLBrace());
    test15.push_back(TokenBuilder::makeRBrace());

    //runTest("Test 1: Minimal valid config", test1);
    //runTest("Test 2: Multiple arguments", test2);
    //runTest("Test 3: Missing semicolon", test3);
    //runTest("Test 4: Directive with only one word", test4);
    //runTest("Test 5: Missing opening brace", test5);
    //runTest("Test 6: Missing closing brace", test6);
    //runTest("Test 7: Location outside server", test7);
    //runTest("Test 8: Empty config", test8);
    //runTest("Test 9: Directive outside server", test9);
    //runTest("Test 10: Extra closing brace", test10);
    //runTest("Test 11: Invalid keyword", test11);
    //runTest("Test 12: Location without path", test12);
    //runTest("Test 13: Nested location", test13);
    //runTest("Test 14: Location without directive", test14);
    runTest("Test 15: Location without closing brace", test15);
    
    return 0;
}




/*
#include <iostream>
#include "config/ConfigLoader.hpp"
#include "config/ConfigErrors.hpp"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file_path>" << std::endl;
        return 1;
    }
    const std::string configPath = argv[1];
    try {
        ConfigLoader::load(configPath);
    } catch (const ConfigError& e) {
        std::cerr << "Config Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
*/