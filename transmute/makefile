SRC_FILES = global.cpp arch.cpp lib.cpp object.cpp netlist.cpp
SRC = $(addprefix checker_legacy/, $(SRC_FILES))
SRC += solver/solverObject.cpp solver/solver.cpp
SRC += main.cpp
OBJ = $(SRC:.cpp=.o)

INCLUDE_PATHS = -I. -Ichecker_legacy -Isolver
LEMON = -I/usr/local/include -L/usr/local/lib -lemon

default: $(OBJ)
	g++ -Wall -Wextra -std=c++11 -g $(INCLUDE_PATHS) $(LEMON) -o build $(OBJ)

%.o: %.cpp
	g++ -Wall -Wextra -std=c++11 -g $(INCLUDE_PATHS) $(LEMON) -c $< -o $@

clean:
	rm -f build.exe $(OBJ)