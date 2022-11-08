CXX=clang++ -std=c++17
CFLAGS= -g -O3 `llvm-config --cppflags --system-libs --libs all` \
-Wno-unused-function -Wno-unknown-warning-option -fno-exceptions -fno-rtti
LDFLAGS = `llvm-config --cppflags --ldflags --system-libs --libs all`

OBJDIR = ./build
TARGET := mccomp
# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard *.cpp)
SRCS := $(filter-out test_driver.cpp, $(SRCS))
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

all: $(TARGET)
$(TARGET): $(addprefix $(OBJDIR)/, $(OBJS))
	$(CXX) -o $@ $^ $(LDFLAGS)

$(addprefix $(OBJDIR)/, %.o): %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJDIR)/*.o *.o
	
# .PHONY: all clean



# mccomp: mccomp.cpp
# 	$(CXX) $(CFLAGS) -o mccomp mccomp.cpp $(LDFLAGS)

# clean:
# 	rm -rf *.o
	