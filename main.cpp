#include "server.hpp"

int main(int ac, char **av, char **env) {
	if (ac == 2) {
		Server server(av[1], env);
		if (server.setup_server() == true)
			server.loop();
	}
	else
		std::cout << "arg error" << std::endl;

	return 0;
}
