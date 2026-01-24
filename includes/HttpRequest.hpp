/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 19:05:01 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/24 19:41:56 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <fstream>

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string version;
};

std::string extractPath(const std::string &request)
{
    std::istringstream iss(request);
    std::string method;
    std::string path;
    std::string version;

    if (!(iss >> method >> path >> version))
        return ""; // invalid request

    return path; // only the requested URL path
}

bool fileExists(const std::string &path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && !S_ISDIR(st.st_mode));
}

bool readFile(const std::string &path, std::string &out)
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file)
        return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

std::string buildHttpResponse(const std::string &body, bool ok = true)
{
    std::ostringstream oss;
    if (ok)
    {
        oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Length: " << body.length() << "\r\n";
        oss << "Content-Type: text/html\r\n"; // you can improve with MIME later
        oss << "Connection: close\r\n";
        oss << "\r\n";
        oss << body;
    }
    else
    {
        std::string msg = "<h1>404 Not Found</h1>";
        oss << "HTTP/1.1 404 Not Found\r\n";
        oss << "Content-Length: " << msg.length() << "\r\n";
        oss << "Content-Type: text/html\r\n";
        oss << "Connection: close\r\n";
        oss << "\r\n";
        oss << msg;
    }
    return oss.str();
}

#endif
