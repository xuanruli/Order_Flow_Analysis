
CXX = g++

CXXFLAGS = -std=c++11 -Wall

TARGET = reconstruction_xuanruli

SRCS = reconstruction_xuanruli.cpp order_book.cpp
TEST_SRC = tests.cpp

OBJS = $(SRCS:.cpp=.o)
TEST_OBJ = $(TEST_SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp order_book.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

TEST_TARGET = tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJ) order_book.o
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJ) order_book.o

$(TEST_OBJ): $(TEST_SRC) order_book.h
	$(CXX) $(CXXFLAGS) -c $(TEST_SRC)

clean:
	rm -f $(TARGET) $(OBJS) $(TEST_TARGET) $(TEST_OBJ) mbp_reconstruction.csv 