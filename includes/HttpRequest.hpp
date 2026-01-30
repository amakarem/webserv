/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 19:05:01 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/30 19:45:08 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <fstream>

class HttpRequest
{
    private:
        std::string method;
        std::string path;
        std::string version;
        bool keepAlive;
        std::string getMimeType();

    public:
        HttpRequest(const std::string &request);
        ~HttpRequest();
        std::string getPath() const;
        std::string getMethod() const ;
        std::string getVersion() const;
        bool isKeepAlive() const;
        std::string buildHttpResponse(const std::string &body, bool ok, size_t fileSize = 0);
};

#endif
