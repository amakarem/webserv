/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 19:05:01 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/13 17:32:22 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <fstream>
#include <filesystem>
#include <unistd.h>

class HttpRequest
{
private:
    std::string method;
    std::string path;
    std::string version;
    bool keepAlive;
    std::string getMimeType();
    std::string raw;
    bool headersComplete;
    bool requestComplete;
    size_t contentLength;
    std::string contentType;
    std::string tmpFileName;
    std::string tmpdir;
    std::ofstream tmpFile;
    size_t bodyReceived;
    std::string cgiHeaders;

public:
    HttpRequest();
    bool append(const char *data, size_t len);
    HttpRequest(const std::string &request);
    ~HttpRequest();
    void setTmpDir(std::string tmpdir);
    void decodePath();
    std::string getPath() const;
    std::string getMethod() const;
    std::string getVersion() const;
    bool isKeepAlive() const;
    bool isHeadersComplete() const;
    bool isRequestComplete() const;
    void setRequestComplete();
    bool isPost() const;
    size_t getContentLength() const;
    size_t getBodyReceived() const;
    std::string getContentType() const;
    std::string getHttpCodeMsg(int httpCode);
    std::string buildHttpResponse(const std::string &body, int httpCode, size_t fileSize = 0);
    void setcgiHeaders(std::string _cgiHeaders);
    std::string getcgiHeaders();
    void reset();
    std::string gettmpFileName();
    std::string getBody();
};

#endif
