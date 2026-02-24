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
    int HttpStatusCode;
    uint64_t contentLength;
    uint64_t MaxContentLength;
    std::string contentType;
    std::string tmpFileName;
    std::string tmpdir;
    std::ofstream tmpFile;
    uint64_t bodyReceived;
    std::string cgiHeaders;
    std::string rawCookieHeader;

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
    bool isInvalidRequest() const;
    size_t getContentLength() const;
    size_t getBodyReceived() const;
    std::string getContentType() const;
    std::string getHttpCodeMsg(int httpCode);
    std::string buildHttpResponse(const std::string &body, size_t fileSize = 0);
    void setcgiHeaders(std::string _cgiHeaders);
    std::string getcgiHeaders();
    void reset();
    std::string gettmpFileName();
    int getHttpStatusCode() const;
    uint64_t parsecontentLength(const std::string &line);
    void setRequestError(int err);
    void setMaxContentLength(uint64_t _MaxContentLength);
    uint64_t getMaxContentLength();
    std::string getrawCookieHeader() const;
};

#endif
