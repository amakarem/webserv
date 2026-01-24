/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:42:01 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/24 21:21:34 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "Client.hpp"

class Server
{
    private:
        // std::vector<int> clients;
        std::vector<Client *> clients;
        int listenFd;
        int port;
        std::string rootdir;
        std::string index;

    public:
        Server();
        Server(char const *filename);
        ~Server();
        Server(Server const &src);
        Server &operator=(Server const &src);
        void setdefaultConf();
        void setConfig(char const *filename);
        void validateConfig();
        void startListening();
        void run();
        std::string resolvePath(const std::string &path);
        
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
