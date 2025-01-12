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
make run1       # run case1
make run7       # run case7
make run10
```

- How to build and run the checker program. If our result doesn't have legality problem, the checker program will terminate without error, and show the wirelength result at last.
```
cd eda_2024_track8_checker
make clean
make
make run1       # check case1
make run7       # check case7
make run10
```

## code structure
All of the solver program is implemented in `transmute` directory. Transmute means things changes radically, we sincerely hope our placement solver can improve wirelength dramatically!
