# **************************************************************************** #
#                          ft_irc — Week 1 (Network)                           #
#   poll 루프 + accept + 라인 에코 스켈레톤                                     #
# **************************************************************************** #

NAME		= ircserv

CXX			= c++
INCLUDES	= -I include
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 $(INCLUDES) -MMD -MP

# ── 소스 (include/ 와 미러링되는 src/ 구조) ──────────────────────────────
SRC_DIR		= src
OBJ_DIR		= obj

SRCS		= \
	main.cpp \
	common/Utils.cpp \
	server/Server.cpp \
	server/ServerAuth.cpp \
	server/Servercmds.cpp \
	server/PollManager.cpp \
	server/Socket.cpp \
	client/Client.cpp \
	parser/Message.cpp \
	parser/Parser.cpp \
	parser/commands/Nick.cpp \
	parser/commands/Pass.cpp \
	parser/commands/User.cpp \
	parser/commands/Ping.cpp \
	parser/commands/Pong.cpp \
	parser/commands/Quit.cpp \
	channel/Channel.cpp \
	channel/commands/Join.cpp \
	channel/commands/Part.cpp \
	channel/commands/Invite.cpp \
	channel/commands/Mode.cpp \
	channel/commands/Kick.cpp \
	channel/commands/Topic.cpp \
	channel/commands/Privmsg.cpp

# src/server/Server.cpp -> obj/server/Server.o (하위 구조 유지)
OBJS		= $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
DEPS		= $(OBJS:.o=.d)

# ── 규칙 ──────────────────────────────────────────────────────────────────
all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "==> $(NAME) 빌드 완료"

# 각 .cpp -> obj/.../.o (변경된 것만 재컴파일 -> 불필요한 relink 없음)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	find $(SRC_DIR) -type f \( -name "*.o" -o -name "*.d" \) -delete 2>/dev/null || true
	@echo "==> 오브젝트 및 의존성 파일 삭제"

fclean: clean
	rm -f $(NAME) test_parser
	@echo "==> $(NAME) 삭제"

re: fclean all

# ── 개인 테스트 전용 규칙 (선택 실행) ─────────────────────────────────────
TEST_NAME   = test_parser
TEST_SRCS   = $(filter-out main.cpp, $(SRCS))

# 1. 표준 RFC 1459 테스트 (단위 테스트 + 표준 통합 테스트)
test: $(NAME)
	@echo "==> Parser 단위/통합 테스트 빌드 중..."
	$(CXX) $(CXXFLAGS) tests/parser/test_parser.cpp $(addprefix $(SRC_DIR)/, $(TEST_SRCS)) -o $(TEST_NAME)
	@echo "==> 1. $(TEST_NAME) 실행"
	./$(TEST_NAME)
	@echo "==> 2. test_integration.py (RFC 1459 규격) 실행"
	python3 tests/test_integration.py

# 2. Irssi 클라이언트 시뮬레이션 전용 테스트
test_irssi: $(NAME)
	@echo "==> test_integration_irssi.py (Irssi 클라이언트) 실행"
	python3 tests/test_integration_irssi.py

-include $(DEPS)

.PHONY: all clean fclean re test test_irssi



