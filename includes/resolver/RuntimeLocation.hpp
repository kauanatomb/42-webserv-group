#pragma once
#include <map>
#include <vector>
#include <string>
#include <set>
#include "RuntimeStruct.hpp"

class RuntimeLocation {
    private:
        std::string path;
        std::string root;
        std::vector<std::string> index;
        size_t client_max_body_size;
        std::map<int, std::string> error_pages;
        bool autoindex;
        std::set<HttpMethod> allow_methods;
        bool has_return;
        ReturnRule redirect;
        bool has_upload;
        std::string upload_store;
        bool has_cgi;
        std::map<std::string, std::string> cgi_exec;
    public:
        RuntimeLocation(const std::string& nodePath);
        // Setters
        void setRoot(const std::string& value);
        void setIndex(const std::vector<std::string>& values);
        void setClientMaxBodySizeLoc(size_t value);
        void addErrorPageLoc(int status, const std::string& path);
        void mergeErrorPage(const std::map<int, std::string>& parent_error_pages);
        void setUploadStore(const std::string& value);
        void setCGI(const std::vector<std::string>& args);
        
        // Methods for directive processing
        void changeStatus(const std::string& directive_name);
        void methodsHTTP(const std::vector<std::string>& methods);
        void setReturn(const std::vector<std::string>& args);
        
        // Getters
        const std::string& getPath() const;
        const std::string& getRoot() const;
        const std::vector<std::string>& getIndex() const;
        size_t getClientMaxBodySize() const;
        const std::map<int, std::string>& getErrorPages() const;
        bool getAutoindex() const;
        const std::set<HttpMethod>& getAllowedMethods() const;
        bool getHasReturn() const;
        const ReturnRule& getRedirect() const;
        bool getHasUpload() const;
        const std::string& getUploadStore() const;
        bool getHasCGI() const;
        const std::map<std::string, std::string>& getCGIExec() const;
};
