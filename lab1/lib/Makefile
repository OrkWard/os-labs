C_SRC = printk.c
OBJ   = printk.o

all: $(OBJ)

%.o:%.c
	${GCC} ${CFLAG} -c $<
clean:
	$(shell rm *.o 2>/dev/null)
