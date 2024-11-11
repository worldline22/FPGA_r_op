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

g++ -Wall -Wextra -std=c++11 -g -I. -Ichecker_legacy -Isolver -I/usr/local/include -L/usr/local/lib -lemon -o build1 ../transmute/checker_legacy/global.o ../transmute/checker_legacy/arch.o ../transmute/checker_legacy/lib.o ../transmute/checker_legacy/object.o ../transmute/checker_legacy/netlist.o ../transmute/solver/solverObject.o ../transmute/solver/solver.o ism_solver.o

extern int xyz_2_index(int x, int y, int z, bool isLUT){
    return isLUT ? (y * 150 + x) * 32 + z * 2 : (y * 150 + x) * 32 + z * 2 + 1;
}

extern int index_2_z_inst(int index){
    return (index % 32) / 2;
}

extern int index_2_x_inst(int index)
{
    return (index / 32) % 150;
}

extern int index_2_y_inst(int index)
{
    return index / 4800;
}

extern int xyz_2_index(int x, int y, int z, bool isLUT);
extern int index_2_x_inst(int index);
extern int index_2_y_inst(int index);
extern int index_2_z_inst(int index);