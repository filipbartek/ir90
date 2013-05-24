# Compilation settings:
CFLAGS=-Wall -std=gnu99 -I/usr/include/netpbm -fopenmp $(MYCFLAGS)
LDFLAGS=-lm -lnetpbm -pthread $(MYLDFLAGS)

OPTCFLAGS=$(CFLAGS) -O3 -march=native -DNDEBUG
DBGCFLAGS=$(CFLAGS) -ggdb3 -msse

# Benchmarking settings:
RUNS=4
ITERS=20

# Hamming distance:
COMPARE=./hamming

######


.PHONY: all clean benchmark evaluate asm test
all: aim-opt aim-dbg


aim-opt: aim-run.c exercise.c
	$(CC) $(OPTCFLAGS) -o $@ $^ $(LDFLAGS) 

aim-dbg: aim-run.c exercise.c
	$(CC) $(DBGCFLAGS) -o $@ $^ $(LDFLAGS)


exercise-opt.s: exercise.c
	$(CC) $(OPTCFLAGS) -fverbose-asm -S $< -o $@

exercise-dbg.s: exercise.c
	$(CC) $(DBGCFLAGS) -fverbose-asm -S $< -o $@

asm: exercise-dbg.s exercise-opt.s


clean:
	rm -f *.o *.s aim-opt aim-dbg output.pbm


benchmark: aim-opt
	@{ echo; \
		top -b -n 5 | head -n 5; \
		echo; \
		echo '>>> Will compute time needed for $(ITERS) iterations averaged over $(RUNS) runs.'; \
		echo '>>> The measured mean time, its probable lower and upper bounds and S.D.'; \
		echo '>>> are printed out. See the header of statistics.awk for details.'; } >&2
	@(for i in `seq 1 $(RUNS)`; do \
		./aim-opt $(ITERS) test16384.pbm output.pbm | tee /dev/stderr; \
	done) | awk -f statistics.awk

evaluate: aim-opt
	for s in 64 1024; do ./aim-opt 1 test$$s.pbm output.pbm; done >/dev/null
	# Consider the optimistic estimate
	make -s benchmark | { read m o p s; echo $$m $$o $$p $$s >&2; echo $$o; }

test: aim-opt
	./aim-opt 1 test1024.pbm test1024-out.pbm
	$(COMPARE) 1 test1024-out.pbm r1024.pbm
	./aim-opt 1 test64.pbm test64-out.pbm
	$(COMPARE) 1 test64-out.pbm test64-ref.pbm
	./aim-opt 1 test16384.pbm test16384-out.pbm
	$(COMPARE) 1 test16384-out.pbm test16384-ref.pbm
