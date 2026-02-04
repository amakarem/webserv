/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Struct.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 23:10:27 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/04 23:11:10 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STRUCT_HPP
# define STRUCT_HPP

#include <string>
#include <vector>
#include <map>

struct ServerConfig {
    std::string ip = "0.0.0.0";                 // listen interface
    int port = 8080;                             // listen port
    std::string root;                            // root directory
    std::vector<std::string> indexFiles;         // index files in order
    std::string serverName;                      // optional server name
};

#endif