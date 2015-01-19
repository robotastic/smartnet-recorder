ulimit -c unlimited

#valgrind --tool=memcheck --leak-check=full ./smartnet --freq 856187500.0 --center 857600000.0  --rate 5000000 --error -5300 --rfgain 14 --ifgain 20 --bbgain 20

./smartnet --freq 854862500.0 --center 858000000.0  --rate 8000000 --error 13900 --rfgain 14 --ifgain 24 --bbgain 24
