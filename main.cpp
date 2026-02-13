/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:26:46 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/13 17:16:09 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        std::cerr << "Usage: ./webserv [config_file]\n";
        return (1);
    }
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    Server server;
    try
    {
        if (argc == 1)
            server.setdefaultConf();
        else if (argc == 2)
            server.setConfig(argv[1]);
        server.validateConfig();
        server.startListening();
        server.run();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what();
    }
    return (0);
}
