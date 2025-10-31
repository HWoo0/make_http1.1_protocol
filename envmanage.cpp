#include "envmanage.hpp"

Envmanage::Envmanage(char **env) {
	for (int i = 0; env[i]; i++) {
		std::string env_str(env[i]);
		size_t eq_pos = env_str.find('=');
		if (eq_pos != std::string::npos) {
			std::string key = env_str.substr(0, eq_pos);
			std::string value = env_str.substr(eq_pos + 1);
			envp[key] = value;
		}
	}
}

void Envmanage::set_env(std::string key, std::string value) {
	envp[key] = value;
}

void Envmanage::unset_env(std::string key) {
	envp.erase(key);
}

char** Envmanage::get_env() {
	str.clear();
	exe_env.clear();
	
	for (std::map<std::string, std::string>::const_iterator it = envp.begin();
			it != envp.end(); ++it) {
		str.push_back(it->first + "=" + it->second);
	}
	
	exe_env.reserve(str.size() + 1);
	for (size_t i = 0; i < str.size(); i++) {
		exe_env.push_back(const_cast<char*>(str[i].c_str()));
	}
	exe_env.push_back(NULL);
	
	return &exe_env[0];
}
