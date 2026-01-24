/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 19:05:01 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/24 22:32:59 by aelaaser         ###   ########.fr       */
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
        std::string getMimeType();

    public:
        HttpRequest(const std::string &request);
        ~HttpRequest();
        std::string getPath();
        std::string getMethod();
        std::string getVersion();
        std::string buildHttpResponse(const std::string &body, bool ok, size_t fileSize = 0);
};

#endif
