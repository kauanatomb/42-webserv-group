# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ktombola <ktombola@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/10 10:18:02 by ktombola          #+#    #+#              #
#    Updated: 2025/11/10 10:18:06 by ktombola         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CXX = c++
CXX_FLAGS = -Wall -Wextra -Werror -std=c++98 -I includes
INFILE = src/main.cpp src/config/ConfigParser.cpp src/debug/ASTPrinter.cpp

OBJ = $(INFILE:.cpp=.o)

NAME = webserv

all: $(NAME)

$(NAME):$(OBJ)
	$(CXX) $(CXX_FLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

## pending: put object files to obj folder of config code