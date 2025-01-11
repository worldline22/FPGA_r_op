SRC = global.cpp util.cpp rsmt.cpp arch.cpp lib.cpp object.cpp netlist.cpp legal.cpp wirelength.cpp pindensity.cpp main.cpp
OBJ = $(SRC:.cpp=.o)
CC = g++

CFLAGS = -Wall -Wextra -std=c++11
CFLAGS += -g

all: checker

checker: $(OBJ)
	$(CC) $(CFLAGS) -o checker $(OBJ)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f checker $(OBJ)

run1:
	./checker ../result/Runfile/case1.run

run2:
	./checker ../result/Runfile/case2.run

run3:
	./checker ../result/Runfile/case3.run

run4:
	./checker ../result/Runfile/case4.run

run5:
	./checker ../result/Runfile/case5.run

run6:
	./checker ../result/Runfile/case6.run

run7:
	./checker ../result/Runfile/case7.run

run8:
	./checker ../result/Runfile/case8.run

run9:
	./checker ../result/Runfile/case9.run

run10:
	./checker ../result/Runfile/case10.run
