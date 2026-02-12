CXX = g++
CXXFLAGS = -Wall -fdiagnostics-color=always -O3
SRC = life.cpp life_cl_view.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = life

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
