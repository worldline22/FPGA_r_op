# Detailed Placement for FPGA wirelength optimization

## how to build and run
- environment set up: linux environment needed. Third party library `Lemon` should be installed: https://lemon.cs.elte.hu/trac/lemon/wiki/Downloads.


- how to build the solver program (output program is called `build`)
```
cd transmute
make clean
make
```

- With binary program `build`, how to run a specific case. We have not complete dynamic strategy for automatic iteration number decision, so you may check and edit the `config.txt` according to `result/record.xlsx` before running a case.
```
cd transmute
make run7       # run case7
make run10      # run case10
```

- How to build and run the checker program. If our result doesn't have legality problem, the checker program will terminate without error, and show the wirelength result at last.
```
cd eda_2024_track8_checker
make clean
make
make run7       # check case7
make run10      # check case10
```

## code structure
All of the solver program is implemented in `transmute` directory. Transmute means things changes radically, we sincerely hope our placement solver can improve wirelength dramatically!

We use codes in `checker_legacy` to parse the input files. These file are copied from official checker, we its data structure and input logic, and move the data to our own data structure in `solver`.

`solver/solverObject.cpp` includes data structure, such as STile, SInstance, SNet and SPin. It initializes solver and provides necessary information. It also includes force-oriented computation.

`solver/wirelength.cpp` links FLUTE solver and gives cost function in ISM Solving.

`solver/solver.cpp` is written for Bank-level coarse-grained ISM. `solver/solverI.cpp` is written for Instance-level fine-grained ISM. The function being executed in parallel is `realizeMatching` and `instanceWLdifference`, including cost computing and ISM solving.


## Supplementary, Contribution Details

Contribution in our team is proportional to commit record. Zhan Chen decides the main workflow and completes the framework of algorithm (such as iterative computing, update mechanism). Yuchao Qin finishes the ISM computing kernel (such as Independent Set Constructing, Matching). Haotong Zhang completes the parallelism in the algorithm and made some detailed modification.
