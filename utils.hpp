#pragma once

#include <string>
#include <sstream>
#include <stdint.h>
#include <cstdlib>
#include <map>
#include <vector>
#include <sys/stat.h>

unsigned int ip_trans(const std::string& str);
std::string find_boundary(std::string &request);
std::string find_host(const std::string& request);
long long find_length(std::string &request);
long cal_size(std::string &str);
std::string findLongMatch(const std::string& input, const std::map<std::string, std::map<std::string, std::vector<std::string> > >& Map);
bool is_directory(const std::string& path);
