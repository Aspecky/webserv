NAME := webserv

SRC_DIR := ./src
INC_DIR := ./include
BUILD_DIR_REL := ./build/release
BUILD_DIR_DEV := ./build/dev
BUILD_DIR_TEST := ./build/test

TEST_DIR := ./tests

HEADERS := $(shell find $(SRC_DIR) $(INC_DIR) -type f -name "*.hpp")
SRCS := $(shell find $(SRC_DIR) -type f -name "*.cpp")
SRC_NO_MAIN := $(filter-out $(SRC_DIR)/main.cpp, $(SRCS))
TEST_SRCS := $(shell find $(TEST_DIR) -name "*.cpp" 2>/dev/null)

CXX := clang++
CXXFLAGS_COMMON := -std=c++98 -Wall -Wextra -I$(INC_DIR)
CXXFLAGS_REL := -Werror
CXXFLAGS_DEV := -Wformat=2 -Wpedantic -Wconversion -fsanitize=address -ggdb3 -O0

all: BUILD_DIR := $(BUILD_DIR_REL)
all: CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_REL)
all: $(BUILD_DIR_REL)/$(NAME)

dev: BUILD_DIR := $(BUILD_DIR_DEV)
dev: CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_DEV)
dev: $(BUILD_DIR_DEV)/$(NAME)

$(BUILD_DIR_REL)/$(NAME): OBJS := $(SRCS:%.cpp=$(BUILD_DIR_REL)/%.o)
$(BUILD_DIR_REL)/$(NAME): $(SRCS:%.cpp=$(BUILD_DIR_REL)/%.o)
	$(CXX) $(OBJS) $(CXXFLAGS) -o $@
	@ln -sf $(BUILD_DIR_REL)/$(NAME) .

$(BUILD_DIR_DEV)/$(NAME): OBJS := $(SRCS:%.cpp=$(BUILD_DIR_DEV)/%.o)
$(BUILD_DIR_DEV)/$(NAME): $(SRCS:%.cpp=$(BUILD_DIR_DEV)/%.o)
	$(CXX) $(OBJS) $(CXXFLAGS) -o $@
	@ln -sf $(BUILD_DIR_DEV)/$(NAME) .

$(BUILD_DIR_REL)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR_DEV)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_DEV) -I$(TEST_DIR)/include
test: $(BUILD_DIR_TEST)/$(NAME)_test
	./$(BUILD_DIR_TEST)/$(NAME)_test

$(BUILD_DIR_TEST)/$(NAME)_test: OBJS := $(SRC_NO_MAIN:%.cpp=$(BUILD_DIR_TEST)/%.o) $(TEST_SRCS:%.cpp=$(BUILD_DIR_TEST)/%.o)
$(BUILD_DIR_TEST)/$(NAME)_test: $(SRC_NO_MAIN:%.cpp=$(BUILD_DIR_TEST)/%.o) $(TEST_SRCS:%.cpp=$(BUILD_DIR_TEST)/%.o)
	$(CXX) $(OBJS) $(CXXFLAGS) -o $@

$(BUILD_DIR_TEST)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean test
clean:
	rm -rf ./build

fclean: clean
	rm -f $(NAME)

re: fclean all

print-%:
	@echo $($*)
