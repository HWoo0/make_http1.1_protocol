CC = c++
NAME = webserv
SRC = main.cpp server.cpp parser.cpp utils.cpp envmanage.cpp
OBJ = $(SRC:.cpp=.o)
CFLAG = -Wall -Wextra -Werror -std=c++98
HEADER = ./

all : $(NAME)

$(NAME) : $(OBJ)
	$(CC) $(CFLAG) -I$(HEADER) $(SRC) -o $(NAME)

clean :
	/bin/rm -f $(OBJ)

fclean : clean
	/bin/rm -f $(NAME)

re : fclean all

rclean : re clean

.PHONY : all clean fclean re rclean
