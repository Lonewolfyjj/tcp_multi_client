CROSS_COMPILE = arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
TARGET = ./$(notdir $(CURDIR))
Lib = -lpthread -lrt
INCDIR := ./ ./fifo ./hup ./led_drv ./uart_mod ./msg_queue_mod\
./uart_drv ./cmd_ctrl_led ./timer_drv ./hip_mod ./tcp_mod ./app 

Include := $(foreach n, $(INCDIR), -I $(n))
SRC = $(foreach n, $(INCDIR), $(wildcard $(n)/*.c))

OBJS = $(patsubst %.c, %.o, $(SRC))

$(TARGET) : $(OBJS)
	$(CC) $^ -o $@ $(Lib) $(Include)
	$(RM) $(OBJS)
%.o : %.c
	@$(CC) -c $< -o $@ $(Include)

.PHONY: clean
clean:
	$(RM) $(OBJS) $(TARGET)

#find . -type f | xargs -n 5 touch