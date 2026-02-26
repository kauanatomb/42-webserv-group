#include "resolver/RuntimeLocation.hpp"
#include <cstdlib>

RuntimeLocation::RuntimeLocation(const std::string& nodePath) : path(nodePath), client_max_body_size(0), autoindex(false), has_return(false), has_upload(false), has_cgi(false) {}

// Setters

void RuntimeLocation::setRoot(const std::string& value) { root = value; }

void RuntimeLocation::setIndex(const std::vector<std::string>& values) {
    index = values;
}

void RuntimeLocation::setClientMaxBodySizeLoc(size_t value) {
    client_max_body_size = value;
}

static std::string joinPath(const std::string& base, const std::string& path) {
    if (base.empty())
        return path;
    if (path.empty())
        return base;

    if (base.back() == '/' && path.front() == '/')
        return base + path.substr(1);

    if (base.back() != '/' && path.front() != '/')
        return base + "/" + path;

    return base + path;
}

void RuntimeLocation::addErrorPageLoc(int status, const std::string& path) {
    error_pages[status] = joinPath(root, path);
}

void RuntimeLocation::mergeErrorPage( const std::map<int, std::string>& parent, const std::string& server_root) {
    for (std::map<int, std::string>::const_iterator it = parent.begin();
        it != parent.end(); ++it) {
        if (error_pages.count(it->first) == 0)
            error_pages[it->first] = joinPath(server_root, it->second);
    }
}

void RuntimeLocation::setUploadStore(const std::string& value) {
    upload_store = value;
}

void RuntimeLocation::setCGI(const std::vector<std::string>& args) {
    // Format: cgi_exec .extension /path/to/cgi/binary
    // args[0] = extension, args[1] = path
    if (args.size() >= 2) {
        cgi_exec[args[0]] = args[1];
    }
}

// Directive processing methods
void RuntimeLocation::changeStatus(const std::string& directive_name) {
    if (directive_name == "autoindex")
        autoindex = true;
    else if (directive_name == "upload")
        has_upload = true;
    else if (directive_name == "cgi")
        has_cgi = true;
}

void RuntimeLocation::methodsHTTP(const std::vector<std::string>& methods) {
    for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
        if (*it == "GET")
            allow_methods.insert(GET);
        else if (*it == "POST")
            allow_methods.insert(POST);
        else if (*it == "DELETE")
            allow_methods.insert(DELETE);
    }
}

void RuntimeLocation::setReturn(const std::vector<std::string>& args) {
    if (args.empty())
        return;
    
    has_return = true;
    redirect.status_code = std::atoi(args[0].c_str());
    redirect.target = args.size() > 1 ? args[1] : "";
}

// Getters
const std::string& RuntimeLocation::getPath() const { return path; }

const std::string& RuntimeLocation::getRoot() const { return root; }

const std::vector<std::string>& RuntimeLocation::getIndex() const { return index; }

size_t RuntimeLocation::getClientMaxBodySize() const { return client_max_body_size; }

const std::map<int, std::string>& RuntimeLocation::getErrorPages() const { return error_pages; }

bool RuntimeLocation::getAutoindex() const { return autoindex; }

const std::set<HttpMethod>& RuntimeLocation::getAllowedMethods() const { 
    return allow_methods; 
}

bool RuntimeLocation::getHasReturn() const { return has_return; }

const ReturnRule& RuntimeLocation::getRedirect() const { return redirect; }

bool RuntimeLocation::getHasUpload() const { return has_upload; }

const std::string& RuntimeLocation::getUploadStore() const { return upload_store; }

bool RuntimeLocation::getHasCGI() const { return has_cgi; }

const std::map<std::string, std::string>& RuntimeLocation::getCGIExec() const { 
    return cgi_exec; 
}
