# Detailed Placement for FPGA wirelength optimization

## how to build and run
- how to build the solver program
```
cd transmute
make clean
make
```

- how to run a specific case
```
cd transmute
make run1       # run case1
make run2       # run case2
...
make run10
```

- how to build and run the checker program
```
cd eda_2024_track8_checker
make clean
make
make run1       # check case1
make run2       # check case2
...
make run10
```

## code structure
All of the solver program is implemented in `transmute` directory. Transmute means things changes radically, we sincerely hope our placement solver can improve wirelength dramatically!
