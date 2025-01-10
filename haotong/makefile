SRC_FILES = global.cpp arch.cpp lib.cpp object.cpp netlist.cpp rsmt.cpp
SRC = $(addprefix checker_legacy/, $(SRC_FILES))
SRC += solver/solverObject.cpp solver/solver.cpp solver/solverI.cpp solver/wirelength.cpp
SRC += helper/debug.cpp
SRC += main.cpp
OBJ = $(SRC:.cpp=.o)

INCLUDE_PATHS = -I. -Ichecker_legacy -Isolver -Ihelper
LEMON = -I/usr/local/include -L/usr/local/lib -lemon
PTHREAD = -lpthread  # add pthread

default: $(OBJ)
	g++ -Wall -Wextra -std=c++20 -g -O0 $(INCLUDE_PATHS) -o build $(OBJ) $(LEMON) $(PTHREAD)

%.o: %.cpp
	g++ -Wall -Wextra -std=c++20 -g -O0 $(INCLUDE_PATHS) $(LEMON) -c $< -o $@

clean:
	rm -f build $(OBJ)

run:
	./build ../benchmark/case_1.nodes ../benchmark/case_1.nets ../benchmark/case_1.timing case_1.nodes.out

run1:
	./build ../benchmark/case_1.nodes ../benchmark/case_1.nets ../benchmark/case_1.timing ../../public/Test/MRes/case_1.nodes.out

run2:
	./build ../benchmark/case_2.nodes ../benchmark/case_2.nets ../benchmark/case_2.timing ../../public/Test/MRes/case_2.nodes.out

run3:
	./build ../benchmark/case_3.nodes ../benchmark/case_3.nets ../benchmark/case_3.timing ../../public/Test/MRes/case_3.nodes.out

run4:
	./build ../benchmark/case_4.nodes ../benchmark/case_4.nets ../benchmark/case_4.timing ../../public/Test/MRes/case_4.nodes.out

run5:
	./build ../benchmark/case_5.nodes ../benchmark/case_5.nets ../benchmark/case_5.timing ../../public/Test/MRes/case_5.nodes.out

run6:
	./build ../benchmark/case_6.nodes ../benchmark/case_6.nets ../benchmark/case_6.timing ../../public/Test/MRes/case_6.nodes.out

run7:
	./build ../benchmark/case_7.nodes ../benchmark/case_7.nets ../benchmark/case_7.timing ../../public/Test/MRes/case_7.nodes.out

run8:
	./build ../benchmark/case_8.nodes ../benchmark/case_8.nets ../benchmark/case_8.timing ../../public/Test/MRes/case_8.nodes.out

run9:
	./build ../benchmark/case_9.nodes ../benchmark/case_9.nets ../benchmark/case_9.timing ../../public/Test/MRes/case_9.nodes.out

run10:
	./build ../benchmark/case_10.nodes ../benchmark/case_10.nets ../benchmark/case_10.timing ../../public/Test/MRes/case_10.nodes.out

runall:
	./build ../benchmark/case_1.nodes ../benchmark/case_1.nets ../benchmark/case_1.timing ../../public/Test/MRes/case_1.nodes.out
	./build ../benchmark/case_2.nodes ../benchmark/case_2.nets ../benchmark/case_2.timing ../../public/Test/MRes/case_2.nodes.out
	./build ../benchmark/case_3.nodes ../benchmark/case_3.nets ../benchmark/case_3.timing ../../public/Test/MRes/case_3.nodes.out
	./build ../benchmark/case_4.nodes ../benchmark/case_4.nets ../benchmark/case_4.timing ../../public/Test/MRes/case_4.nodes.out
	./build ../benchmark/case_5.nodes ../benchmark/case_5.nets ../benchmark/case_5.timing ../../public/Test/MRes/case_5.nodes.out
	./build ../benchmark/case_6.nodes ../benchmark/case_6.nets ../benchmark/case_6.timing ../../public/Test/MRes/case_6.nodes.out
	./build ../benchmark/case_7.nodes ../benchmark/case_7.nets ../benchmark/case_7.timing ../../public/Test/MRes/case_7.nodes.out
	./build ../benchmark/case_8.nodes ../benchmark/case_8.nets ../benchmark/case_8.timing ../../public/Test/MRes/case_8.nodes.out
	./build ../benchmark/case_9.nodes ../benchmark/case_9.nets ../benchmark/case_9.timing ../../public/Test/MRes/case_9.nodes.out
	./build ../benchmark/case_10.nodes ../benchmark/case_10.nets ../benchmark/case_10.timing ../../public/Test/MRes/case_10.nodes.out