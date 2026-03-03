## Grammar rules for parsing Configuration File
config        → server*
server        → "server" "{" (directive | location)* "}"
location      → "location" WORD "{" directive* "}"
directive     → WORD WORD* ";"

### Parsing output

Generic example
Config
└── server
    ├── directives
    │   ├── listen
    │   ├── server_name
    │   ├── root
    │   └── index
    └── locations
        ├── location /
        │   └── directives
        ├── location /upload
        │   └── directives
        └── location /cgi-bin
            └── directives

### Concrete Example of multiple servers
ConfigAST
├── ServerNode (8080)
│   └── locations
└── ServerNode (8081)
    └── locations


## Grammar rules for HTTP request

Request = Request-Line Host-Header *(Other-Header) CRLF [Message-Body]
-----------------------------------------------------------------
Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
Header-Field   = Field-Name ":" [Field-Value] CRLF
Message-Body   = *OCTET
----------------------------------
Host-Header    = 
Method         = "GET" | "POST" | "DELETE"
Request-URI    = absoluteURI | abs_path
abs_path       = "/" *( segment )
segment        = *pchar
HTTP-Version   = "HTTP/" 1*DIGIT "." 1*DIGIT
Field-Name     = token
Field-Value    = *( any-char )
SP             = <space character (0x20)>
CRLF           = <carriage return (0x0D) + line feed (0x0A)>

## Grammar rules for Chunked data
Message-Body        = Chunked-Body

Chunked-Body        = 1*Chunk Last-Chunk

Chunk               = Chunk-Size CRLF
                      Chunk-Data CRLF

Last-Chunk          = "0" CRLF CRLF

Chunk-Size          = 1*HEXDIG

Chunk-Data          = *OCTET  ; length defined by Chunk-Size