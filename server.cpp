#include "server.hpp"
#include "utils.hpp"

Server::Server(char *av, char **env): envp(env) {
	std::vector<Block> block = block_pars(av, parser);
	processConfigBlocks(parser, block);
	printParser(parser);
}

bool Server::setup_server() {
	set_port();
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		std::cerr << "epoll_create1 error" << std::endl;
		return false;
	}
	if (setup_socket() == false)
		return false;
	std::cout << "서버 실행중" << std::endl;
	return true;
}

void Server::set_port() {
	std::set<std::pair<std::string, int> > unique;
	for (size_t i = 0; i < parser.server_block.size(); i++) {
		std::map<std::string, std::vector<std::string> >::iterator it = parser.server_block[i].begin();
		for (; it != parser.server_block[i].end(); ++it) {
			if (it->first == "listen") {
				std::string add = it->second[0];
				size_t pos = add.find(":");
				std::string ip;
				std::string port;
				if (pos == std::string::npos) {
					ip = "0.0.0.0";
					port = add;
				}
				else {
					ip = add.substr(0, pos);
					port = add.substr(pos+1);
				}
				std::istringstream iss(port);
				int num;
				iss >> num;
				std::pair<std::string, int> address(ip, num);
                if (unique.find(address) == unique.end()) {
                    unique.insert(address);
                    IP.push_back(ip);
                    PORT.push_back(num);
                }
			}
		}
	}
}

bool Server::setup_socket() {
	for (size_t i = 0; i < PORT.size(); i++) {
		int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (server_fd == -1) {
			std::cerr << "socket error" << std::endl;
			return false;
		}
	
		int opt = 1;
		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
			std::cerr << "setsockopt error" << std::endl;
			close(server_fd);
			return false;
		}
	
		sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(ip_trans(IP[i]));
		server_addr.sin_port = htons(PORT[i]);
	
		if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
			std::cerr << "bind error" << std::endl;
			return false;
		}
	
		if (listen(server_fd, SOMAXCONN) == -1) {
			std::cerr << "listne error" << std::endl;
			return false;
		}

		memset(&event, 0, sizeof(event));
		event.data.fd = server_fd;
		event.events = EPOLLIN;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
			std::cerr << "epoll ctl ADD error" << std::endl;
			close(server_fd);
			return false;
		}
		server_fds.push_back(server_fd);
	}
	return true;
}

void Server::loop() {
	while (true) {
		int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		for (int i = 0; i < n; i++) {
			size_t j = 0;
			int fd = events[i].data.fd;
			
			for (; j < PORT.size(); j++) {
				if (server_fds[j] == fd) {
					client_connect(j);
					break;
				}
			}
			if (j == PORT.size()) {
				client_fd = fd;
				if (events[i].events & EPOLLIN) {
					client_read_hadnle();  // 읽기
				}
				if (events[i].events & EPOLLOUT) {
					client_write_handle(fd); // 쓰기
				}
				if (events[i].events & (EPOLLERR | EPOLLHUP)) {
					close_client(fd); // 연결 종료
				}
			}
		}
	}
	for (int j = 0; j < PORT.size(); j++)
		close(server_fds[j]);
}

void Server::client_connect(int num) {
	while (true) {
		sockaddr_in client_addr;
		memset(&client_addr, 0, sizeof(client_addr));
		socklen_t client_len = sizeof(client_addr);
		int client_fd = accept(server_fds[num], (sockaddr*)&client_addr, &client_len);
		if (client_fd == -1)
			break;
		clients.insert(client_fd);
		
		epoll_event client_event;
		memset(&client_event, 0, sizeof(client_event));
		client_event.data.fd = client_fd;
		client_event.events = EPOLLIN;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
			std::cerr << "epoll ctl ADD error" << std::endl;
			close(client_fd);
			break;
		}
		std::cout << "클라이언트 접속됨 (fd: " << client_fd << ")" << std::endl;
	}
}

void Server::handle_cgi_request(std::string input, std::string exe, std::string request, std::string resource) {
	int pipefd[2], pipefd_in[2];
	std::string add_var;
	if (pipe(pipefd) == -1 || pipe(pipefd_in) == -1) {
		send_error("500", "Internal Server Error", resource);
		return;
	}

	pid_t pid = fork();
	if (pid < 0) {
		return;
	}
	
	else if (pid == 0) {
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd_in[0], STDIN_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		close(pipefd_in[1]);
		close(pipefd_in[0]);

		set_cgi_env(input, exe, request, resource);

		char *argv[] = {const_cast<char*>(exe.c_str()), NULL};
        execve(exe.c_str(), argv, envp.get_env());

		std::cerr << "execve error" << std::endl;
		exit(1);
	}
	else {
		close(pipefd[1]);
		close(pipefd_in[0]);

		ssize_t written = write(pipefd_in[1], input.c_str(), input.size());
        if (written == -1) {
            std::cerr << "CGI write error" << std::endl;
            close(pipefd_in[1]);
            close(pipefd[0]);
            kill(pid, SIGTERM);
            waitpid(pid, NULL, 0);
            send_error("500", "Internal Server Error", resource);
            return;
        }
		close(pipefd_in[1]);

		char buffer[1024];
		ssize_t n;
		std::string cgi_output;

		while (true) {
			n = read(pipefd[0], buffer, sizeof(buffer));
			if (n == -1) {
				std::cerr << "CGI read error" << std::endl;
				close(pipefd[0]);
				kill(pid, SIGTERM);
				waitpid(pid, NULL, 0);
				send_error("500", "Internal Server Error", resource);
				return;
			}
			if (n == 0)
				break;
			
			cgi_output.append(buffer, n);
		}

		close(pipefd[0]);
		waitpid(pid, NULL, 0);

		if (cgi_output.empty())
			send_error("404","Not Found",resource);
		else
			send_200(cgi_output, true);
	}
}

void Server::set_cgi_env(std::string input, std::string exe, std::string request, std::string resource) {
	std::string option = "OFF";
	if (exe.find("upload_cgi") != std::string::npos) {
		size_t pos = request.find("Content-Type: ");
		std::string type = request.substr(pos+14);
		size_t pos2 = type.find('\n');
		type = type.substr(0, pos2);
		envp.set_env("CONTENT_TYPE", type);
		if (parser.location_block[idx][resource].find("upload_enable") != parser.location_block[idx][resource].end())
			option = parser.location_block[idx][resource]["upload_enable"][0];
	}
	if (option == "ON" || option == "on") {
		std::string upload_store = "./upload";
		if (parser.location_block[idx][resource].find("upload_store") != parser.location_block[idx][resource].end())
			upload_store = parser.location_block[idx][resource]["upload_store"][0];
		envp.set_env("UPLOAD_STORE", upload_store);
	}
	else
		envp.unset_env("UPLOAD_STORE");

	envp.set_env("REQUEST_METHOD", "POST");

	std::stringstream ss;
	ss << input.size();
	std::string len_str = ss.str();

	envp.set_env("CONTENT_LENGTH", len_str);
}

std::string Server::generate_autoindex_html(const std::string& path, const std::string& request_uri, const std::string & location) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
		send_error("403", "Forbidden", location);
		return "";
	}

    std::string html = "<html><body>";
    html += "<h1>Index of " + request_uri + "</h1><ul>";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "../")
			continue;
        if (entry->d_type == DT_DIR)
			name += "/";

        html += "<li><a href=\"" + name + "\">" + name + "</a></li>";
    }

    html += "</ul></body></html>";
    closedir(dir);
    return html;
}

void Server::close_client(int fd) {
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
		std::cerr << "epoll ctl DEL error" << std::endl;
    close(fd);
    clients.erase(fd);
    request.erase(fd);
    response_buffer.erase(fd);
}

bool Server::is_chunked_complete(const std::string& body) {
	size_t pos = 0;
	while (pos < body.size()) {
		size_t crlf = body.find("\r\n", pos);
		if (crlf == std::string::npos)
			return false; // 청크 크기 못 찾음
		std::string hex = body.substr(pos, crlf - pos);
		size_t chunk_size = strtol(hex.c_str(), NULL, 16);
		if (chunk_size == 0) { // 마지막 청크
			if (pos + 2 <= body.size() && body.substr(pos, 2) == "\r\n") { // crlf 확인
				return true;
			}
			return false;
		}
		pos = crlf + 2; // 청크 데이터 시작
		if (pos + chunk_size + 2 > body.size()) {
			return false; // 데이터 부족
		}
		pos += chunk_size + 2; // 청크 데이터 + \r\n 건너뛰기
	}
	return false; // 0 청크를 못 찾음
}

std::string Server::dechunk(const std::string& body) {
	std::string result;
	size_t pos = 0;
	while (pos < body.size()) {
		size_t crlf = body.find("\r\n", pos);
		if (crlf == std::string::npos)
			break;
		std::string hex = body.substr(pos, crlf - pos);
		size_t chunk_size = strtol(hex.c_str(), NULL, 16);
		pos = crlf + 2;
		if (chunk_size == 0)
			break;
		result += body.substr(pos, chunk_size);
		pos += chunk_size + 2;
	}
	return result;
}

void Server::client_write_handle(int fd) {
	std::string &data = response_buffer[fd];
	ssize_t sent = send(fd, data.c_str(), data.size(), 0);
	
	if (sent == -1) {
		std::cerr << "send failed" << std::endl;
		close_client(fd);
		return;
	}
	if (sent == 0) {
		std::cerr << "connect close" << std::endl;
		close_client(fd);
		return;
	}
	data.erase(0, sent);

	if (data.empty()) {
		response_buffer.erase(fd);
		
		epoll_event ev;
		ev.data.fd = fd;
		ev.events = EPOLLIN;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
			std::cerr << "epoll ctl MOD error" << std::endl;
			close_client(fd);
			return;
		}
		request[fd].clear();
		keep_alive[fd] = true;
	}
}

void Server::client_read_hadnle() {
	char buffer[BUFFER_SIZE + 1] = {0};
	ssize_t count;
	count = recv(client_fd, buffer, BUFFER_SIZE, 0);
	if (count == -1) {
		std::cerr << "recv error" << std::endl;
		close_client(client_fd);
		return;
	}
	if (count == 0) {
		std::cout << "클라이언트 연결 종료됨 (fd: " << client_fd << ")\n";
		close_client(client_fd);
		return;
	}
	request[client_fd].append(buffer, count);

	if (!is_request_complete()) {
        std::cout << "요청 미완성" << std::endl;
        return;
    }

	process_complete_request();
}

bool Server::is_request_complete() {
    size_t head_pos = request[client_fd].find("\r\n\r\n");
    if (head_pos == std::string::npos) {
        return false; // 헤더 미완성
    }
    
    // chunked
    if (request[client_fd].find("Transfer-Encoding: chunked") != std::string::npos) {
        return is_chunked_complete(request[client_fd].substr(head_pos + 4));
    }
    
    // content length
    long long content_length = find_length(request[client_fd]);
    if (content_length >= 0) {
        size_t expected_total = head_pos + 4 + content_length;
        return request[client_fd].size() >= expected_total;
    }
    
    return true;
}

void Server::select_block() {
	idx = 0;  // 기본 서버
    std::string host = find_host(request[client_fd]);
    if (host.empty()) {
        send_error("400", "Bad Request", "");
        return;
    }

	std::string host_name = host;
    size_t colon_pos = host.find(":");
    if (colon_pos != std::string::npos) {
        host_name = host.substr(0, colon_pos);
    }
	
    for (size_t i = 0; i < parser.server_block.size(); i++) {
        if (parser.server_block[i].find("server_name") != parser.server_block[i].end()) {
            std::string server_name = parser.server_block[i]["server_name"][0];
            if (server_name == host_name) {
                idx = i;
                return ;
            }
        }
	}

    for (size_t i = 0; i < parser.server_block.size(); i++) {
        std::map<std::string, std::vector<std::string> >::iterator it = parser.server_block[i].find("listen");
        if (it != parser.server_block[i].end()) {
            std::string addr = it->second[0];
            size_t pos = addr.find(":");
            std::string port = (pos == std::string::npos) ? addr : addr.substr(pos + 1);
            size_t host_pos = host.find(":");
            std::string host_port = (host_pos != std::string::npos) ? host.substr(host_pos + 1) : "";
			if (port == host_port ) {
                idx = i;
				break ;
            }
        }
    }
}

void Server::redirect_handle(std::string location) {
	std::string url;
	if (parser.location_block[idx][location].find("return") != parser.location_block[idx][location].end()) {
		std::string code = parser.location_block[idx][location]["return"][0];
		std::istringstream iss(code);
		int num = 0;
		iss >> num;
		if (num / 100 == 4)
			send_error("404","Not Found", location);
		else if (num / 100 == 3)
			send_300(location);
	}
}

void Server::post_handle(std::string location, std::string body) {
	bool method_allowed = true;

	if (parser.location_block[idx][location].find("methods") != parser.location_block[idx][location].end()) {
		std::vector<std::string>& methods = parser.location_block[idx][location]["methods"];
		method_allowed = (std::find(methods.begin(), methods.end(), "POST") != methods.end());
	}
	if (!method_allowed)
		send_error("405", "Method Not Allowed", location);

	if (parser.location_block[idx][location].find("cgi_pass") != parser.location_block[idx][location].end()) {
		std::string cgi_pass = parser.location_block[idx][location]["cgi_pass"][0];
		handle_cgi_request(body, cgi_pass, request[client_fd], location); //cgi_pass ./cgi-bin/upload_cgi
	}
	else
		send_error("405", "Method Not Allowed", location);
}

void Server::get_handle(std::string location, std::string resource, std::string method) {
	bool send_body = (method == "GET");

	bool method_allowed = true;

	if (parser.location_block[idx][location].find("methods") != parser.location_block[idx][location].end()) {
		std::vector<std::string>& methods = parser.location_block[idx][location]["methods"];
		method_allowed = (std::find(methods.begin(), methods.end(), "GET") != methods.end());
	}
	if (!method_allowed) {
		send_error("405", "Method Not Allowed", location);
		return;
	}

	std::string filename;
	std::string root = "./";
	std::string index = "index.html";

	if (parser.location_block[idx][location].find("root") != parser.location_block[idx][location].end()) {
		root = parser.location_block[idx][location]["root"][0];
		if (root[root.size()-1] != '/')
			root += '/';
	}
	if (parser.location_block[idx][location].find("index") != parser.location_block[idx][location].end()) {
		index = parser.location_block[idx][location]["index"][0];
	}
	if (!resource.empty() && resource[0] == '/')
		resource = resource.substr(1);
	filename = root + resource;
	if (is_directory(filename)) {
		if (filename[filename.size()-1] != '/')
			filename += '/';
		filename += index;
	}
	std::ifstream file(filename.c_str());
	if (!file.is_open()) {
		std::string option = "OFF";
		if (parser.location_block[idx][location].find("autoindex") != parser.location_block[idx][location].end()) {
			option = parser.location_block[idx][location]["autoindex"][0];
		}
		if (option == "ON" || option == "on") {
			std::string html = generate_autoindex_html(root, resource, location);
			if (!html.empty()) {
				send_200(html, send_body);
			}
		}
		else
			send_error("404","Not Found", location);
	}
	else {
		std::string html((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		send_200(html, send_body);
	}
}

void Server::delete_hendle(std::string resource) {
	std::string root = "./";  // 기본 루트

	std::string key = findLongMatch("/delete", parser.location_block[idx]);
	if (parser.location_block[idx][key].find("root") != parser.location_block[idx][key].end())
		root = parser.location_block[idx][key]["root"][0];

	size_t pos = resource.find("delete_");
	resource = resource.substr(pos+7);
	std::string target_path = root + "/" + resource;

	if (access(target_path.c_str(), F_OK) != 0) {
		send_error("404","Not Found", key); // 파일 없음
		return;
	}

	if (access(target_path.c_str(), W_OK) != 0) {
		send_error("403", "Forbidden", key); // 권한 없음
		return;
	}

	if (std::remove(target_path.c_str()) != 0) {
		send_error("403", "Forbidden", key); // 실패
		return;
	}

	std::string filename = "./html/delete_success.html";
	std::ifstream file(filename.c_str());
	if (!file.is_open()) {
		std::cerr << "file open error" << filename << std::endl;
		send_error("404","Not Found", key);
		return;
	}
	std::string html((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	send_200(html, true);
}

void Server::process_complete_request() {
	size_t head_pos = request[client_fd].find("\r\n\r\n");

	long client_max_body_size = 5 * 1024 * 1024;
    std::map<std::string, std::vector<std::vector<std::string> > >::iterator it = parser.global.begin();
    for (; it != parser.global.end(); it++) {
		if (it->first == "client_max_body_size") {
			client_max_body_size = cal_size(it->second[0][0]);
		}
	}

    std::string body;
    if (request[client_fd].find("Transfer-Encoding: chunked") != std::string::npos) {
        body = dechunk(request[client_fd].substr(head_pos + 4));
    }
	else {
        long long content_length = find_length(request[client_fd]);
        if (content_length > client_max_body_size) {
            send_error("413", "Payload Too Large", "");
            return;
        }
        if (content_length > 0) {
            body = request[client_fd].substr(head_pos + 4, content_length);
        }
    }
    
    if (body.size() > (size_t)client_max_body_size) {
        send_error("413", "Payload Too Large", "");
        return;
    }

	// 요청 처리 시작
	request_handle(body);
}

void Server::request_handle(std::string body) {
	std::cout << "[클라이언트 " << client_fd << "] 받은 요청:\n" << request[client_fd] << std::endl;
	size_t pos1 = request[client_fd].find('\n');
	std::string req = request[client_fd].substr(0, pos1);
	std::istringstream iss(req);
    std::string word;
    std::vector<std::string> tokens;

	keep_alive[client_fd] = true;
	if (request[client_fd].find("Connection: close") != std::string::npos)
		keep_alive[client_fd] = false;

    select_block();

    while (iss >> word) {
        tokens.push_back(word);
	}
	if (tokens.size() < 2) {
		send_error("404","Not Found", "");
		request.erase(client_fd);  // 요청 내용 초기화
		return;
	}
	std::string method = tokens[0];
	std::string resource = tokens[1];
	size_t qpos = resource.find('?');
	if (qpos != std::string::npos)
		resource = resource.substr(0, qpos);

	std::string location = findLongMatch(resource, parser.location_block[idx]);

	if (parser.location_block[idx][location].find("return") != parser.location_block[idx][location].end()) {
        redirect_handle(location);
    }
	else if (method == "POST") {
		post_handle(location, body);
	}
	else if (method == "GET" || method == "HEAD") {
		get_handle(location, resource, method);
	}
	else if (method == "DELETE") {
		delete_hendle(resource);
	}
	else
		send_error("405", "Method Not Allowed", location);

	request.erase(client_fd);  // 요청 내용 초기화
}

void Server::send_200(std::string &html, bool body) {
	std::stringstream ss;
	ss << html.size();
	std::string len_str = ss.str();

	std::string Connection = keep_alive[client_fd] ? "keep-alive" : "close";
	std::string content_type = "text/html";
	std::string response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: " + content_type + "\r\n"
		"Content-Length: " + len_str + "\r\n"
		"Connection: " + Connection + "\r\n"
		"\r\n";
	if (body) {
		response += html;
	}

	response_buffer[client_fd] = response;
	if (!response_buffer[client_fd].empty()) {
		epoll_event ev;
		ev.data.fd = client_fd;
		ev.events = EPOLLOUT;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev) == -1) {
			std::cerr << "epoll ctl MOD error" << std::endl;
			close_client(client_fd);
			return;
		}
	} // out 이벤트로 변경
}

void Server::send_300(std::string location) {
	std::string url = parser.location_block[idx][location]["return"][1];
	std::string Connection = keep_alive[client_fd] ? "keep-alive" : "close";
	std::string redirect_response =
		"HTTP/1.1 302 Found\r\n"
		"Location: " + url + "\r\n"
		"Content-Length: 0\r\n"
		"Connection: " + Connection + "\r\n"
		"\r\n";

	response_buffer[client_fd] = redirect_response;
	if (!response_buffer[client_fd].empty()) {
		epoll_event ev;
		ev.data.fd = client_fd;
		ev.events = EPOLLOUT;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev) == -1) {
			std::cerr << "epoll ctl MOD error" << std::endl;
			close_client(client_fd);
			return;
		}
	} // out 이벤트로 변경
}

std::string Server::find_error_page(std::string num, std::string location) {
	std::string name = "./error/400.html"; // error_page
	if (!location.empty()) {
		std::map<std::string, std::vector<std::string> >::iterator end_location = parser.location_block[idx][location].end();
		std::map<std::string, std::vector<std::string> >::iterator it_location = parser.location_block[idx][location].find("error_page");
		std::map<std::string, std::vector<std::string> >::iterator end_server = parser.server_block[idx].end();
		std::map<std::string, std::vector<std::string> >::iterator it_server = parser.server_block[idx].find("error_page");
		std::map<std::string, std::vector<std::vector<std::string> > >::iterator end_global = parser.global.end();
		std::map<std::string, std::vector<std::vector<std::string> > >::iterator it_global = parser.global.find("error_page");
		if (it_location != end_location) {
			int len = it_location->second.size();
			for (int i = 0; i < len; i++) {
				std::string str = it_location->second[i];
				if (str == num)
					name = it_location->second[len-1];
			}
		}
		else if (it_server != end_server) {
			int len = it_server->second.size();
			for (int i = 0; i < len; i++) {
				std::string str = it_server->second[i];
				if (str == num)
					name = it_server->second[len-1];
			}
		}
		else if (it_global != end_global) {
			int len = it_global->second.size();
			for (int i = 0; i < len; i++) {
				int n = it_global->second[i].size();
				for (int j = 0; j < n -1; j++) {
					std::string str = it_global->second[i][j];
					if (str == num)
						name = it_global->second[i][n-1];
				}
			}
		}
	}
	return name;
}

void Server::send_error(std::string num, std::string mes, std::string location) {
	std::string name = find_error_page(num, location);
	std::ifstream file(name.c_str());
	std::string html;
	if (!file.is_open())
        html = "<html><body><h1>" + num + " " + mes + "</h1></body></html>";
	else
        html.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::stringstream ss;
	ss << html.size();
	std::string len_str = ss.str();
	std::string Connection = keep_alive[client_fd] ? "keep-alive" : "close";
	std::string error =
		"HTTP/1.1 " + num + " " + mes + "\r\n"
		"Content-Length: " + len_str + "\r\n"
		"Content-Type: text/html\r\n"
		"Connection: " + Connection + "\r\n"
		"\r\n" + html;
		
	response_buffer[client_fd] = error;
	if (!response_buffer[client_fd].empty()) {
		epoll_event ev;
		ev.data.fd = client_fd;
		ev.events = EPOLLOUT;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev) == -1) {
			std::cerr << "epoll ctl MOD error" << std::endl;
			close_client(client_fd);
			return;
		}
	} // out 이벤트로 변경
} 
