ulimit -c unlimited

#valgrind --tool=memcheck --leak-check=full ./smartnet --freq 856187500.0 --center 857600000.0  --rate 5000000 --error -5300 --rfgain 14 --ifgain 20 --bbgain 20

./smartnet --freq 856187500.0 --center 858000000.0  --rate 8000000 --error 14300 --rfgain 14 --ifgain 25 --bbgain 25
