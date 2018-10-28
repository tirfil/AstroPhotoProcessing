

CC = g++
CFLAGS = -fpermissive 
LDFLAGS =  
LIB = -lcfitsio

TARGET = center
SRCS = $(TARGET).cpp image.cpp
OBJS = $(SRCS:.cpp=.o)
 

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) $(OBJS) $(LIB) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< 
