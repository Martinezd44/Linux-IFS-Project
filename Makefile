CC = gcc
CFLAGS=
LDFLAGS = 
OBJFILES= ifs.o disk.o main.o 
TARGET = ifs

all:$(TARGET)
$(TARGET) : $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET)