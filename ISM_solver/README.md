# This is our ISM project

self.data_cls = DataCollections(
            params, placedb, dtype=self.dtype, device=self.device
        )

pos_xyz = loc_xyz[
                self.data_cls.movable_range[0] : self.data_cls.movable_range[1]
            ]

self.movable_range = placedb.movableRange()

g++ ism_solver.cpp -o ism_solver -std=c++17 -I/usr/local/include -L/usr/local/lib -lemon

g++ -Wall -Wextra -std=c++11 -g -I. -Ichecker_legacy -Isolver -I/usr/local/include -L/usr/local/lib -lemon -c ism_solver.cpp -o ism_solver.o

g++ -Wall -Wextra -std=c++11 -g -I. -Ichecker_legacy -Isolver -I/usr/local/include -L/usr/local/lib -lemon -o build ../transmute/checker_legacy/global.o ../transmute/checker_legacy/arch.o ../transmute/checker_legacy/lib.o ../transmute/checker_legacy/object.o ../transmute/checker_legacy/netlist.o ../transmute/solver/solverObject.o ../transmute/solver/solver.o ../transmute/main.o ism_solver.o