*This project has been created as part of the 42 curriculum by aelaaser, blohrer, frocha

# WebServ

## Description
WebServ is a custom HTTP web server developed as part of the 42 curriculum.  
The goal of this project is to implement a fully functional web server that can handle multiple server configurations, serve static files, support CGI scripts, manage error pages, and handle file uploads.  

WebServ allows multiple servers to run simultaneously on different ports, each with its own configuration, making it flexible for development and testing environments.  

## Instructions

### Compilation
Compile the project using the provided Makefile:

```bash
make
```

### Execution

You can run WebServ in two ways:

1- Without a configuration file (default behavior) it will load the defualt config file config/webserv.conf:

```bash
./webserv
```

2- With a custom configuration file:

```bash
./webserv configfile.conf
```

## Configuration File

WebServ configuration is done through a config file defining one or more servers. Each server block should include the following:

### Required Parameters

- `listen` — defines the IP and port the server listens on

- `root` — the root directory for the server

- `index` — default index files

###  Optional Parameters

- `server_name` — domain name of the server

- `autoindex` — enable directory listing (on/off or 1/0)

- `error_page` — path to custom error pages example (error_page 404 /var/www/error_pages/404.html;)

- `allowupload` — configure upload paths, it's 2 parts the endpoint and the localfolder example (allowupload /upload_request /var/www/uploads;) any POST request to upload_request will save the file into /var/www/uploads

- `php_upload_max_filesize` — maximum file upload size for PHP defualt set to 20M

- `php_post_max_size` — maximum POST request size for PHP defualt set to 20M

- `php_memory_limit` — PHP memory limit for PHP defualt set to 128M

- `allowedMethods` — allowed HTTP methods (GET, POST, DELETE, PUT) if not set all is allowed

- `location` — URL redirects (supports 301 or 302 status codes) example (location /redirect /subfolder 302;)

- `tmpdir` — The server use this folder for storing the tmp files and requests defualt is `/var/tmp`.

**Ensure all server blocks have the required parameters listen, root, and index. All other parameters are optional.**

## Supported CGI
- `php-cgi`

## Notes
- The `www` folder contains sample test files you can use to quickly test the server.

