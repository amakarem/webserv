/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:40:32 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/24 20:56:17 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

std::string HttpRequest::extractPath(const std::string &request)
{
    std::istringstream iss(request);
    std::string method;
    std::string path;
    std::string version;

    if (!(iss >> method >> path >> version))
        return ""; // invalid request

    return path; // only the requested URL path
}

bool HttpRequest::fileExists(const std::string &path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && !S_ISDIR(st.st_mode));
}

bool HttpRequest::readFile(const std::string &path, std::string &out)
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file)
        return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

std::string HttpRequest::buildHttpResponse(const std::string &body, bool ok)
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
