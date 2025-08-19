# Makefile for Modbus DTU project

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# 源文件
SOURCES = main.cpp modbus_rtu.cpp modbus_test.cpp modbus_config.cpp
HEADERS = modbus_rtu.h modbus_test.h modbus_config.h

# 目标文件
OBJECTS = $(SOURCES:.cpp=.o)

# 可执行文件名
TARGET = modbus_dtu

# 构建目录
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin

# 默认目标
all: $(BIN_DIR)/$(TARGET)

# 创建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR): $(BUILD_DIR)
	mkdir -p $(BIN_DIR)

# 编译可执行文件
$(BIN_DIR)/$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@

# 编译对象文件
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理编译文件
clean:
	rm -f $(OBJECTS)
	rm -rf $(BUILD_DIR)

# 重新编译
rebuild: clean all

# 运行程序
run: $(BIN_DIR)/$(TARGET)
	cd $(BIN_DIR) && ./$(TARGET)

# 调试编译
debug: CXXFLAGS += -g -DDEBUG
debug: $(BIN_DIR)/$(TARGET)

# 声明伪目标
.PHONY: all clean rebuild run debug

# 显示帮助信息
help:
	@echo "可用的目标:"
	@echo "  all     - 编译项目 (默认)"
	@echo "  clean   - 清理编译文件"
	@echo "  rebuild - 重新编译"
	@echo "  run     - 编译并运行程序"
	@echo "  debug   - 编译调试版本"
	@echo "  help    - 显示此帮助信息"
