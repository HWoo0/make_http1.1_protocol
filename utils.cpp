#include "utils.hpp"

unsigned int ip_trans(const std::string& str) {
    std::stringstream ss(str);
    std::string token;
    uint32_t result = 0;

    for (int i = 0; i < 4; ++i) {
		std::getline(ss, token, '.');
        std::stringstream conv(token);
        int byte = 0;
        conv >> byte;
        if (conv.fail() || byte < 0 || byte > 255) {
            throw std::runtime_error("Invalid IP byte");
        }
        result = (result << 8) | byte;
    }

    return result;
}

std::string find_boundary(std::string &request) {
	size_t pos = request.find("boundary=");
	if (pos == std::string::npos)
		return "";
	size_t start = pos + 9;
	size_t end = request.find("\n", pos);
	std::string boundary = request.substr(start, (end - 1) - start);
	return boundary;
}

std::string find_host(const std::string& request) {
    size_t pos = request.find("Host: ");
    if (pos == std::string::npos)
		return "";
    size_t end = request.find('\n', pos);
	std::string host = request.substr(pos + 6, end - (pos + 7));
	if (host.find("localhost") != std::string::npos) {
		size_t p = host.find("localhost");
		std::string port = host.substr(p + 9);
		host = "127.0.0.1" + port;
	}
    return host;
}

long long find_length(std::string &request) {
	size_t pos = request.find("Content-Length: ");
	if (pos == std::string::npos)
		return -1;
	size_t end = request.find('\n', pos);
	std::string len = request.substr(pos+16, end - (pos+16));
	return atoll(len.c_str());
}

long cal_size(std::string &str) {
	std::string numpart;
	long K = 1024;
	long M = 1024 * 1024;
	long G = 1024 * 1024 * 1024;
	char unit = '\0';

    for (size_t i = 0; i < str.size(); ++i) {
        if (std::isdigit(str[i])) {
            numpart += str[i];
        }
		else {
            unit = std::toupper(str[i]);
            break;
        }
    }
	std::istringstream iss(numpart);
    long value = 0;
    iss >> value;
    if (iss.fail())
        {return -1;}
	if (unit == 'K')
		{return value * K;}
	if (unit == 'M')
		{return value * M;}
	if (unit == 'G')
		{return value * G;}
	return value;
}

std::string findLongMatch(const std::string& input, const std::map<std::string, std::map<std::string, std::vector<std::string> > >& Map) {
    std::string Match;
    for (std::map<std::string, std::map<std::string, std::vector<std::string> > >::const_iterator it = Map.begin(); it != Map.end(); ++it) {
        const std::string& key = it->first;
        if (input.compare(0, key.size(), key) == 0) {
            if (key.size() > Match.size()) {
                Match = key;
            }
        }
    }
    return Match;
}

bool is_directory(const std::string& path) {
	struct stat info;
	if (stat(path.c_str(), &info) != 0)
		return false;
	return S_ISDIR(info.st_mode);
}
