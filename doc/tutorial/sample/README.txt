For installation, please read the README.txt file.
The default installation directory is /opt/ppos/
Please include paths /opt/ppos/bin and /opt/ppos/sbin/ in your PATH environment
variable.
    PATH=$PATH:/opt/ppos/bin:/opt/ppos/sbin/
    MANPATH=$MANPATH:/opt/ppos/man/

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
(1) How to run
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  a) Boot/reboot McKernel
  Refer to HOW_TO_USE.txt.
  
  b) Make the executable files
  Run "make".

  c) Run the executable files
     (i) All except picomp and pifomp
         Execute the following command.

	 $ sudo mcexec chello

     (ii) picomp and pifomp
          You need to set two environmental variables because they use
          OpenMP. Firstly, you need to set OMP_NUM_THREADS to the
          number less than the number of cores allocated to
          McKernel. Secondly, you need to append the path of the
          shared objects of Intel OpenMP to LD_LIBRARY_PATH. You can
          use the script provided by Intel for this purpose. You can
          perform the both by using the following command.
  
          $ sudo sh -c '. /opt/intel/bin/compilervars.sh intel64 &&\
            OMP_NUM_THREADS=2 mcexec picomp'

