/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:38:20 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/04 23:45:02 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <ctime>
#include <dirent.h>
#include "HttpRequest.hpp"
#include "Struct.hpp"

class Client
{
    private:
        int fd;                      // client socket
        std::ifstream* file;          // pointer to open file
        std::string headerBuffer;     // HTTP headers to send
        bool headersSent;             // headers already sent
        bool finished;                // done sending everything
        bool keepAlive;
        long lastActivity;
        std::string rootDir;
        std::string serverName;
        std::vector<std::string> indexFiles;
        bool autoindex;

    public:
        Client(int _fd, const ServerConfig &config);
        ~Client();
        int getFd() const;

        void setkeepAlive(bool val);
        bool isKeepAlive() const;
        
        void setFile(std::ifstream* f);
        std::ifstream* getFile() const;

        void setHeaderBuffer(const std::string& buf);
        std::string& getHeaderBuffer();

        void setlastActivity();
        long getlastActivity() const;
        bool isTimeout() const;
        
        void setHeadersSent(bool val);
        bool isHeadersSent() const;

        void setFinished(bool val);
        bool isFinished() const;

        std::string resolvePath(const std::string &path);
        std::string generateDirectoryListing(const std::string &dir);
        int readRequest();
        int sendResponse();
};
#endif
