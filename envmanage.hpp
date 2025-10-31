#pragma once

#include <map>
#include <string>
#include <vector>

class Envmanage {
	private:
		std::map<std::string, std::string> envp;
		std::vector<std::string> str;
		std::vector<char*> exe_env;
	public:
		Envmanage(char **env);
		~Envmanage() {};
		void set_env(std::string key, std::string value);
		void unset_env(std::string key);
		char **get_env();
};
