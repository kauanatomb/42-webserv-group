#pragma once

#include "httpCore/HttpRequest.hpp"
#include "httpCore/HttpResponse.hpp"

class RuntimeServer; //forward declare because we only use a pointer 
//in the signature, keeping the resolver headers out of 
//network headers -> fewer circular deps

class RequestHandler 
{
    public: 
        HttpResponse handle(const HttpRequest& req, const RuntimeServer* server );
};




/*
the goal of the RequestHandler.hpp
this header should only expose the public contract:
- it declares calss ReqeustHandler
- it declares handle(const HttpRequest&, cont RuntimeServer*) -> HttpResponse
so it needs to know the types in the signature 
*/

//#include "HttpRequest.hpp" //because my method takes const HttpRequest and the type must be known
//#include "HttpResponse.hpp" //because my method returns HttpResponse by value
/*
it is better to forward declare RuntimeServer, to prevent pulling
resolver includes everywhere that includes 
RequestHandler.hpp (which will be included by Connection, 
ServerEngine, etc) to keep compile time + circular des under control
*/

