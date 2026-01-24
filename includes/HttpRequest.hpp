/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 19:05:01 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/24 20:52:41 by aelaaser         ###   ########.fr       */
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

    public:
        std::string extractPath(const std::string &request);
        bool fileExists(const std::string &path);
        bool readFile(const std::string &path, std::string &out);
        std::string buildHttpResponse(const std::string &body, bool ok = true);
};

#endif
