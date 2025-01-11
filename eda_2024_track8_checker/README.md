# eda_2024_track8_checker
# EDA Contest 2024 Anlogic Checker

1）How to compile the program: 
   cd <path_of_the_src_code>
   make 

2) How to run the checker program:
   The program takes command from a run script file.
   A demo run script can be found in <path_of_the_src_code>/demo.run
   
3) Supported Command 
   3.1) read_arch <*.lib> <*.scl> <*.clk>
        Command to read FPGA device related files.
		.lib: instance models
		.scl: FPGA site information
		.clk: FPAG clock region information		
		
   3.2) report_arch
		Command to report FPGA device related statistics.
				
   3.3) read_design <*.nodes> <*.nets> <*.timing>
        Command to read benchmark related files.
		.nodes: design instances
		.nets: design nets
		.timing: timing critical pins in design
		
   3.4) report_design
   	    Command to report design related statistics.
		
   3.5) read_output	<*.nodes.out>
		Command to read output of optimized instance locations
		
   3.6) legal_check
		Command to perform legalization check, including:
	      a) at any location, if an instance type matches a site type
		  b) is there any over-capacity site 
	      c) check control set constraint
	      d) check clock region constraint
   
   3.7) report_wirelength
        Commend to report wire-length of "*.nodes.out" result
		
   3.8) report_pin_density
        Commend to report pin-density of "*.nodes.out" result
		
   3.9) report_clock_region <clock_col> <clock_row>
        Command to report clock region statistics. 
		This arch has 5x5 clock regions; clock_col ranges from 0 to 4; clock_row ranges from 0 to 4.
   
   3.10) report_tile <col> <row>
		 Command to print tile occupations.	
   
   3.11) report_net <net_name>
         Command to print detailed net wire-length and topology if there is any.
   
   3.12) exit
		 Quit the program.	
  
