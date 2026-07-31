# **************************************************************************** #
#                          ft_irc — Week 1 (Network)                           #
#   poll 루프 + accept + 라인 에코 스켈레톤                                     #
# **************************************************************************** #

NAME		= ircserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
INCLUDES	= -I include

# ── 소스 (include/ 와 미러링되는 srcs/ 구조) ──────────────────────────────
SRC_DIR		= srcs
OBJ_DIR		= obj

SRCS		= \
	main.cpp \
	server/Server.cpp \
	server/PollManager.cpp \
	server/Socket.cpp \
	client/Client.cpp

# srcs/server/Server.cpp -> obj/server/Server.o (하위 구조 유지)
OBJS		= $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))

# ── 규칙 ──────────────────────────────────────────────────────────────────
all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "==> $(NAME) 빌드 완료"

# 각 .cpp -> obj/.../.o (변경된 것만 재컴파일 -> 불필요한 relink 없음)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	@echo "==> 오브젝트 삭제"

fclean: clean
	rm -f $(NAME)
	@echo "==> $(NAME) 삭제"

re: fclean all

.PHONY: all clean fclean re
