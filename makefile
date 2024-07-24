#定义头文件路径
MOSQINCPATH = `pwd`/../install/include

#定义库文件路径
MOSQLIBPATH = `pwd`/../install/lib

INC += -I ${MOSQINCPATH}

LIB += -L ${MOSQLIBPATH}

CC = gcc

all:
	${CC} can_mqtt.c -o can_mqtt ${INC} ${LIB} -lmosquitto

clean:
	rm can_mqtt

can_run:
	./can_mqtt -i can0
