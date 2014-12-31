ulimit -c unlimited

#valgrind --tool=memcheck --leak-check=full ./smartnet --freq 856187500.0 --center 857600000.0  --rate 5000000 --error -5300 --rfgain 14 --ifgain 20 --bbgain 20

#./smartnet --freq 856187500.0 --center 858000000.0  --rate 8000000 --error 13900 --rfgain 14 --ifgain 24 --bbgain 36

## this worked on the 6th floor
#./smartnet --freq 859737500.0 --center 858000000.0  --rate 8000000 --error 4000 --rfgain 14 --ifgain 30 --bbgain 40

## this worked well with HackRF in Needham
# ./smartnet --device 0 --freq 859737500.0 --center 858000000.0 --rate 8000000 --error 4135 --rfgain 14 --ifgain 50 --bbgain 30 --talkgroupfile BostonChannels.csv

## this worked well with USRP (though getting OOOOOOOs)
./smartnet --device 1 --freq 859737500.0 --center 858000000.0  --rate 8000000 --error -500 --ant TX/RX --rfgain 32 --ifgain 50 --bbgain 30 --talkgroupfile BostonChannels.csv

