/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Struct.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 23:10:27 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/13 17:15:43 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STRUCT_HPP
#define STRUCT_HPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

struct Redirect
{
    std::string new_url;
    int code; // e.g. 301, 302
};

struct ServerConfig
{
    std::string ip = "0.0.0.0";          // listen interface
    int port = 8080;                     // listen port
    std::string root;                    // root directory
    std::vector<std::string> indexFiles; // index files in order
    std::string serverName;              // optional server name
    bool autoindex;
    std::string tmpdir = "/var/tmp";
    std::vector<std::string> allowedMethods;
    std::unordered_map<int, std::string> error_pages;
    std::unordered_map<std::string, Redirect> redirects;
    std::unordered_map<std::string, std::string> allowupload;
    std::string php_upload_max_filesize = "20M";
    std::string php_post_max_size = "20M";
    std::string php_memory_limit = "128M";
};

#endif