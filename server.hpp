#pragma once

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <vector>
#include <map>
#include <errno.h>
#include <set>
#include <string>
#include <fstream>
#include <sys/wait.h>
#include <sstream>
#include <cstdlib>
#include <dirent.h>
#include <algorithm>
#include <sys/stat.h>
#include "parser.hpp"
#include "envmanage.hpp"

const int MAX_EVENTS = 64;
const int BUFFER_SIZE = 1024;

class Server {
	private:
		std::vector<int> server_fds;
		std::vector<sockaddr_in> server_addr;
		int client_fd;
		int epoll_fd;
		epoll_event event;
		epoll_event events[MAX_EVENTS];
		std::set<int> clients;
		std::vector<int> PORT;
		std::vector<std::string> IP;
		std::map<int, std::string> request;
		std::map<int, std::string> response_buffer;
		Pas parser;
		Envmanage envp;
		int idx;
		std::map<int, bool> keep_alive;
		bool setup_socket();
		void client_connect(int num);
		void client_read_hadnle();
		void client_write_handle(int fd);
		void process_complete_request();
		bool is_request_complete();
		void request_handle(std::string body);
		void send_200(std::string &html, bool body);
		void send_300(std::string location);
		void handle_cgi_request(std::string input, std::string exe, std::string request, std::string resource);
		void set_cgi_env(std::string input, std::string exe, std::string request, std::string resource);
		void set_port();
		std::string generate_autoindex_html(const std::string& path, const std::string& request_uri, const std::string & location);
		std::string find_error_page(std::string num, std::string location);
		void send_error(std::string num, std::string mes, std::string location);
		void close_client(int fd);
		bool is_chunked_complete(const std::string& body);
		std::string dechunk(const std::string& body);
		void select_block();
		void redirect_handle(std::string location);
		void post_handle(std::string location, std::string body);
		void get_handle(std::string location, std::string resource, std::string method);
		void delete_hendle(std::string resource);
		public:
		Server(char *av, char **env);
		~Server() {};
		bool setup_server();
		void loop();

};
