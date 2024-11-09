SRC_FILES = global.cpp arch.cpp lib.cpp object.cpp netlist.cpp
SRC = $(addprefix checker_legacy\, $(SRC_FILES))
SRC += main.cpp
OBJ = $(SRC:.cpp=.o)

default: $(OBJ)
	g++ -Wall -Wextra -std=c++11 -g -o build $(OBJ)

%.o: %.cpp
	g++ -Wall -Wextra -std=c++11 -g -c $< -o $@

clean:
	del build.exe $(OBJ)