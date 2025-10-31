#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <sstream>
#include <map>
#include <set>


struct Block {
	std::vector<std::string> server;
	std::vector<std::vector<std::string> > location;
};

struct Pas {
	std::map<std::string, std::vector<std::vector<std::string> > > global;
	std::vector<std::map<std::string, std::vector<std::string> > > server_block;
	std::vector<std::map<std::string, std::map<std::string, std::vector<std::string> > > > location_block;
};

bool isAllWhitespace(const std::string &s);
bool isOnlybraces(const std::string &s);
std::string find_uri(std::string &tmp);
std::vector<Block> block_pars(char *av, Pas &parser);
void processConfigBlocks(Pas &parser, std::vector<Block> &block);
void printParser(Pas parser);
