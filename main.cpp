/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:26:46 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/09 21:13:28 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

int main(int argc, char **argv)
{
    if (argc == 1)
        Server server;
    else if (argc == 2)
        Server server(argv[1]);
    else
    {
        std::cerr << "Usage: ./webserv [config_file]\n";
        return 1;
    }
    
    return (0);
}
