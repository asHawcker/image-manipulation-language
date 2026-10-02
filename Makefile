CXX = clang++-22
CXXFLAGS = -std=c++20 -fexceptions
LLVM_FLAGS = $(shell llvm-config-22 --cxxflags --ldflags --system-libs --libs core)

TARGET = iml_compiler

SRCS = lexer.cpp ast.cpp parser.cpp main.cpp semantic.cpp
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LLVM_FLAGS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(LLVM_FLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f output.o output.ll program

run: $(TARGET)
	./$(TARGET) source_code.im 
	clang++ output.o runtime.cpp -o program
	./program