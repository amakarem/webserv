/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:42:01 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/09 21:03:04 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <exception>
#include <map>
#include <algorithm>
#include <cctype>

class Server
{
    private:
        std::map<std::string, std::string>   config;
        int port;
        std::string rootdir;
        std::string index;

    public:
        Server();
        Server(char const *filename);
        ~Server();
        Server(Server const &src);
        Server &operator=(Server const &src);
        void setConfig(char const *filename);
        void validateConfig();
        
        class openFileError : public std::exception
        {
            public:
                const char *what() const throw();
        };
        class invalidPort : public std::exception
        {
            public:
                const char *what() const throw();
        };
        class KeyError : public std::exception
        {
            public:
                const char *what() const throw();
        };
};

#endif