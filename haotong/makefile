SRC_FILES = global.cpp arch.cpp lib.cpp object.cpp netlist.cpp
SRC = $(addprefix checker_legacy/, $(SRC_FILES))
SRC += solver/solverObject.cpp solver/solver.cpp
SRC += solver/global.cpp solver/rsmt.cpp solver/wirelength.cpp
SRC += main.cpp
OBJ = $(SRC:.cpp=.o)

INCLUDE_PATHS = -I. -Ichecker_legacy -Isolver
LEMON = -I/usr/local/include -L/usr/local/lib -lemon
PTHREAD = -lpthread  # add pthread

default: $(OBJ)
	g++ -Wall -Wextra -std=c++11 -g $(INCLUDE_PATHS) -o build $(OBJ) $(LEMON) $(PTHREAD)

%.o: %.cpp
	g++ -Wall -Wextra -std=c++11 -g $(INCLUDE_PATHS) $(LEMON) -c $< -o $@

clean:
	rm -f build.exe $(OBJ)

run:
	./build ../benchmark/case_1.nodes ../benchmark/case_1.nets ../benchmark/case_1.timing ../../public/Test/MRes/case_1.nodes.out
