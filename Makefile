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

re: fclean all

.PHONY: all clean fclean re
