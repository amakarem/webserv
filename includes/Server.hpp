/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:42:01 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/04 22:22:01 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include <map>
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

struct ServerConfig {
    std::string ip = "0.0.0.0";                 // listen interface
    int port = 8080;                             // listen port
    std::string root;                            // root directory
    std::vector<std::string> indexFiles;         // index files in order
    std::string serverName;                      // optional server name
};

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
