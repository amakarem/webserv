/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:38:20 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/04 19:05:39 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <vector>
#include <ctime>
#include "HttpRequest.hpp"

class Client
{
    private:
        int fd;                      // client socket
        std::ifstream* file;          // pointer to open file
        std::string headerBuffer;     // HTTP headers to send
        std::string sendBuffer;
        bool headersSent;             // headers already sent
        bool finished;                // done sending everything
        bool keepAlive;
        long lastActivity;

    public:
        Client(int _fd);
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

        std::string resolvePath(std::string rootdir, std::string index, const std::string &path);
        int readRequest(const std::string &rootDir, const std::string &index);
        int sendResponse();
        int sendResponseIncremental(size_t maxBytes = 1024);
};
#endif
