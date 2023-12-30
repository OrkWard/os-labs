ASM_SRC		= $(sort $(wildcard *.S))
C_SRC       = $(sort $(wildcard *.c))
OBJ		    = $(patsubst %.S,%.o,$(ASM_SRC)) $(patsubst %.c,%.o,$(C_SRC))

all:$(OBJ)


%.o:%.S
	${GCC}  ${CFLAG} -march=rv64g_zifencei -c $<

%.o:%.c
	${GCC}  ${CFLAG} -march=rv64g_zifencei -c $<

clean:
	$(shell rm *.o 2>/dev/null)
