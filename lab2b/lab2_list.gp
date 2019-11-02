#! /usr/bin/gnuplot

#NAME: Erica Xie
#EMAIL: ericaxie@ucla.edu 
#ID: 404920875



# purpose:
#        generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#       1. test name
#       2. # threads
#       3. # iterations per thread
#       4. # lists
#       5. # operations performed (threads x iterations x (ins + lookup + delete))
#       6. run time (ns)
#       7. run time per operation (ns)
#
# output:
#	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations.
#	lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
#	lab2b_3.png ... successful iterations vs. threads for each synchronization method.
#	lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists.
#	lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists.
#



# general plot parameters
set terminal png
set datafile separator ","



# the throughput vs the number of threads for mutex and spin-lock synchronized list operations 
set title "Scalability-1: Throughput vs Number of Threads"
set xlabel "# of Threads"
set logscale x 2
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_1.png'

# grep out only mutex locked and spin locked, non-yield results
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'list ins/lookup/delete w/ mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'list ins/lookup/delete w/ spin' with linespoints lc rgb 'green'



# the wait-for-lock time, and the average time per operation against the number of competing threads
set title "Scalability-2: Per-operation Times for Mutex-Protected List Operations"
set xlabel "# of Threads"
set logscale x 2
set ylabel "mean time/operations"
set logscale y 10
set output 'lab2b_2.png'

# grep out only mutex locked, non-yield results
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
        title 'wait for lock' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
        title 'completion time' with linespoints lc rgb 'green' 



# the number of successful iterations with and without synchronization against the number of competing threads
set title "Scalability-3: Correct Synchronization of Partitioned Lists"
set xlabel "# of Threads"
set logscale x 2
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'

# grep out only mutex locked, spin locked, yield results
plot \
     "< grep 'list-id-none,' lab2b_list.csv | grep '^list'" using ($2):($3) \
        title 'unprotected w/ yields' with points lc rgb 'red', \
     "< grep 'list-id-m,' lab2b_list.csv | grep '^list'" using ($2):($3) \
        title 'mutex w/ yields' with points lc rgb 'green', \
     "< grep 'list-id-s,' lab2b_list.csv | grep '^list'" using ($2):($3) \
        title 'spin w/ yields' with points lc rgb 'blue'



# the throughput vs the number of threads for mutex synchronized partitioned list operations
set title "Scalability-4: Throughput of Mutex-Synchronized Partitioned Lists"
set xlabel "# of Threads"
set logscale x 2
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_4.png'

# grep out only mutex locked results
plot \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '1 list' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,.*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '4 lists' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,.*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '8 lists' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,.*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '16 lists' with linespoints lc rgb 'orange'




# the throughput vs the number of threads for spin-lock synchronized partitioned list operations
set title "Scalability-5: Throughput vs. Throughput of Spin-Lock-Synchronized Partitioned Lists"
set xlabel "# of Threads"
set logscale x 2
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_5.png'

# grep out only spin locked results
plot \
     "< grep 'list-none-s,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '1 list' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,.*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '4 lists' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,.*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '8 lists' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,.*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '16 lists' with linespoints lc rgb 'orange'
