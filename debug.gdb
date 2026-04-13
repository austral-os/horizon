set logging file gdb_output.txt
set logging on
set confirm off
run
echo \n--- BACKTRACE ---\n
bt full
echo \n--- REGISTERS ---\n
info registers
echo \n--- THREADS ---\n
info threads
quit
