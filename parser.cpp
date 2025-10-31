#include "parser.hpp"

const char* directivesArr[] = {
    "client_max_body_size",
    "error_page",
    "listen",
    "server_name",
    "root",
    "index",
    "methods",
    "cgi_pass",
    "upload_enable",
    "upload_store",
    "return",
    "autoindex"
};

const std::set<std::string> validDirectives(
    directivesArr,
    directivesArr + sizeof(directivesArr) / sizeof(directivesArr[0])
);

bool isAllWhitespace(const std::string &s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (!std::isspace(static_cast<unsigned char>(s[i])))
            return false;
    }
    return true;
}
bool isOnlybraces(const std::string &s) {
	for (size_t i = 0; i < s.size(); i++) {
		if (!std::isspace(static_cast<unsigned char>(s[i])) && s[i] != '{' && s[i] != '}')
			return false;
	}
	return true;
}

std::string find_uri(std::string &tmp) {
	std::stringstream str(tmp);
	std::string word;
	std::string res;
	while (str >> word) {
		if (word == "location" || word == "{")
			continue ;
		res = word;
	}
	size_t pos = res.find("location");
	if (pos != std::string::npos)
		res = res.substr(pos+8);
	if (res[res.size()-1] == '{')
		res = res.substr(0, res.size()-1);
	return res;
}

std::vector<Block> block_pars(char *av, Pas &parser) {
	std::string tmp;
	std::ifstream ifile(av);
	std::vector<Block> block;
	std::vector<std::string> global;
	std::vector<std::string> server;
	std::vector<std::vector<std::string> > location;
	std::stack<std::string> st;
	int flag = 0;
	int idx2 = 0;
	int idx3 = 0;
	if (ifile.is_open()) {
		while (getline(ifile, tmp)) {
			if (isAllWhitespace(tmp))
				continue;
			std::istringstream isss(tmp);
			std::string sword;
			while (isss >> sword) {
				if (sword == "server" || sword == "server{") {
					flag = 1;
					server.clear();
					server.push_back(tmp);
					if (tmp.find("{") != std::string::npos)
						st.push("{");
					while (getline(ifile, tmp)) {
						if (isAllWhitespace(tmp))
							continue;
						std::istringstream issl(tmp);
						std::string lword;
						while (issl >> lword) {
							if (lword.find("location") != std::string::npos) {
								if (location.size() <= (size_t)idx2)
									location.resize(idx2 + 1);
								flag = 2;
							}
						}
						if (flag == 2) {
							if (tmp.find("location") != std::string::npos)
								location[idx2].push_back(find_uri(tmp));
							else
								location[idx2].push_back(tmp);
						}
						else {
							server.push_back(tmp);
						}
						if (tmp.find("{") != std::string::npos)
							st.push("{");
						else if (tmp.find("}") != std::string::npos) {
							st.pop();
							if (flag == 2){
								flag = 1;
								idx2++;
							}
						}
						if (st.empty()) 
						{			
							if (block.size() <= (size_t)idx3)
								block.resize(idx3+1);
							block[idx3].server = server;
							block[idx3].location = location;
							idx3++;
							location.clear();
							idx2=0;
							break ;
						}
					}
				}
				else {
					std::string key = sword;
					std::vector<std::string> value;
					while (isss >> sword) {
						if (!sword.empty() && sword[sword.size() - 1] == ';')
    						sword.erase(sword.size() - 1);
						value.push_back(sword);
					}
					if (validDirectives.find(key) == validDirectives.end())
						throw std::runtime_error("config error");
					parser.global[key].push_back(value);
				}
			}
		}
	}
	return block;
}

void processConfigBlocks(Pas &parser, std::vector<Block> &block) {
	parser.server_block.resize(block.size());
	for (size_t j = 0; j < block.size(); ++j) {
		for (size_t k = 1; k < block[j].server.size(); ++k) {
			std::string line = block[j].server[k];
			if (line.empty() || isOnlybraces(line))
				continue;

			if (!line.empty() && line[line.size() - 1] == ';')
    			line.erase(line.size() - 1);

			std::istringstream iss(line);
			std::string key, word;
			iss >> key;
			if (validDirectives.find(key) == validDirectives.end())
				throw std::runtime_error("config error");
			std::vector<std::string> values;
			while (iss >> word)
				values.push_back(word);

			if (!key.empty() && !values.empty())
				parser.server_block[j][key] = values;
		}
	}

	for (size_t j = 0; j < block.size(); j++) {
		std::map<std::string, std::map<std::string, std::vector<std::string> > > tmp;
		for (size_t k = 0; k < block[j].location.size(); k++) {
			std::string key;
			std::map<std::string, std::vector<std::string> > map;
			for (size_t i = 0; i < block[j].location[k].size(); i++) {
				std::string line = block[j].location[k][i];
				if (line.empty() || isOnlybraces(line))
					continue;
				if (i == 0) {
					key = line;
					continue;
				}
				if (!line.empty() && line[line.size() - 1] == ';')
					line.erase(line.size() - 1);
				std::istringstream iss(line);
				std::string lkey, word;
				iss >> lkey;
				if (validDirectives.find(lkey) == validDirectives.end())
					throw std::runtime_error("config error");
				std::vector<std::string> values;
				while (iss >> word)
					values.push_back(word);

				if (!lkey.empty() && !values.empty())
					map[lkey] = values;
			}
			if (!key.empty())
				tmp[key] = map;
		}
		parser.location_block.push_back(tmp);
	}
}

void printParser(Pas parser) {
	std::map<std::string, std::vector<std::vector<std::string> > >::iterator it;
	for (it = parser.global.begin(); it != parser.global.end(); it++) {
        std::cout << it->first << " : " << std::endl;
        int len = it->second.size();
        for (int i = 0; i < len; i++) {
			for (size_t j = 0; j < it->second[i].size(); j++) {
				std::cout << it->second[i][j] << " ";
            }
        	std::cout << std::endl;
        }
    }
	std::cout << std::endl;
	for (size_t i = 0; i < parser.server_block.size(); ++i) {
		for (std::map<std::string, std::vector<std::string> >::iterator it = parser.server_block[i].begin();
			it != parser.server_block[i].end(); ++it) {
			std::cout << it->first << " : ";
			for (size_t j = 0; j < it->second.size(); ++j) {
				std::cout << it->second[j];
				if (j != it->second.size() - 1)
					std::cout << " ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

	std::map<std::string, std::map<std::string, std::vector<std::string> > >::iterator pair;
	for (size_t i = 0; i< parser.location_block.size(); i++) {
		for (pair = parser.location_block[i].begin(); pair != parser.location_block[i].end(); pair++) {
			std::cout << pair->first << " : \n";
			std::map<std::string, std::vector<std::string> >::iterator s;
			for (s = pair->second.begin(); s != pair->second.end(); s++) {
				std::cout << s->first << " : ";
				int len = s->second.size();
				for (int j = 0; j < len; j++) {
					std::cout << s->second[j] << " ";
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}