ifeq ($(MAKECMDGOALS),64)
	CC = x86_64-w64-mingw32-gcc
else
	CC = i686-w64-mingw32-gcc
endif
CFILES = main.c mainwin.c geticons.c listview.c util.c

all:
	$(CC) -g -o winiconview.exe $(CFILES) $(CFLAGS) $(LDFLAGS) $(neededFLAGS)

64: all

# TODO prune the DLLs
neededFLAGS = --std=c99 \
	-luser32 -lkernel32 -lcomctl32 \
	-lcomdlg32 -lgdi32 -lmsimg32 \
	-lshell32 -ladvapi32 -lole32 \
	-lshlwapi
