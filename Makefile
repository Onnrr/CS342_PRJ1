# make command compiles both
# make clean command removes executables

CFLAGS = -Wall -Wextra -pthread

all: threadtopk proctopk

threadtopk: threadtopk.c
	gcc $(CFLAGS) -o threadtopk threadtopk.c

proctopk: proctopk.c
	gcc $(CFLAGS) -o proctopk proctopk.c

clean:
	rm -f threadtopk proctopk