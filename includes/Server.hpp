/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:42:01 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/04 23:11:24 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include <fstream>
#include <iostream>
#include <exception>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include "Client.hpp"
#include "Struct.hpp"

class Server
{
    private:
        std::vector<ServerConfig> serverConfigs;
        std::vector<Client *> clients;
        std::map<int, ServerConfig> listenFdConfig;
        std::vector<int> listenSockets;
        int epollFd;
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
        void disconnectClient(Client *c);
        
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
