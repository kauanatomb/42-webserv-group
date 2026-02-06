# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: falatrac <falatrac@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/10 10:18:02 by ktombola          #+#    #+#              #
#    Updated: 2026/02/06 16:26:32 by falatrac         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CXX = c++
CXX_FLAGS = -Wall -Wextra -Werror -std=c++98 -I includes
NAME = webserv
SRCS = \
	src/main.cpp \
	src/config/Tokenizer.cpp
#INFILE = src/main.cpp

OBJ = $(SRCS:.cpp=.o)

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