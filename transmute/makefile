SRC_FILES = global.cpp arch.cpp lib.cpp object.cpp netlist.cpp
SRC = $(addprefix checker_legacy/, $(SRC_FILES))
SRC += solver/solverObject.cpp solver/solver.cpp
SRC += main.cpp
OBJ = $(SRC:.cpp=.o)

INCLUDE_PATHS = -I. -Ichecker_legacy -Isolver

default: $(OBJ)
	g++ -Wall -Wextra -std=c++11 -g $(INCLUDE_PATHS) -o build $(OBJ)

%.o: %.cpp
	g++ -Wall -Wextra -std=c++11 -g $(INCLUDE_PATHS) -c $< -o $@

clean:
	rm -f build.exe $(OBJ)