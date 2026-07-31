<<<<<<< HEAD
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
=======
# ft_irc — Parser/공통 Phase1 범위 Makefile.
# ircserv 타겟(전체 서버)은 Network/Channel의 src가 아직 없어 Phase2 통합 전까지 만들 수 없다.
# 지금은 Parser를 독립적으로 검증하는 test_parser 타겟만 제공한다(CLAUDE.md 6절 검증 게이트).

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -Iinclude

TEST_NAME	= test_parser

TEST_SRCS	= src/common/Utils.cpp \
			  src/parser/Message.cpp \
			  src/parser/Parser.cpp \
			  src/parser/commands/Pass.cpp \
			  src/parser/commands/Nick.cpp \
			  src/parser/commands/User.cpp \
			  src/parser/commands/Ping.cpp \
			  src/parser/commands/Pong.cpp \
			  src/parser/commands/Quit.cpp \
			  tests/parser/TestDoubles.cpp \
			  tests/parser/test_parser.cpp

TEST_OBJS	= $(TEST_SRCS:.cpp=.o)

all: $(TEST_NAME)

$(TEST_NAME): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(TEST_OBJS) -o $(TEST_NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TEST_OBJS)

fclean: clean
	rm -f $(TEST_NAME)
>>>>>>> feature/parser-07-31

re: fclean all

.PHONY: all clean fclean re
