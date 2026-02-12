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
INFILE = src/main.cpp src/config/ConfigLoader.cpp src/config/Tokenizer.cpp \
			src/config/ConfigParser.cpp src/config/ConfigValidator.cpp 

OBJ_DIR = obj
OBJ = $(INFILE:%.cpp=$(OBJ_DIR)/%.o)

NAME = webserv

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXX_FLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re