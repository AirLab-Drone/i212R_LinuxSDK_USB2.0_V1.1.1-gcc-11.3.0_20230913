CC=g++
TARGET=Demopp
ifeq ($(shell uname -m), x86_64)
	CFLAGS=-I./include -I/usr/local/include/opencv4
	LDFLAGS=-L./lib/x64 -lGuideUSB2LiveStream -lpthread -lm -ludev -lopencv_core -lopencv_highgui -lopencv_imgproc
else
	CFLAGS=-I./include
	LDFLAGS=-L./lib/arrch -lGuideUSB2LiveStream -lpthread -lm -ludev
endif
all: $(TARGET)

$(TARGET): Demo.cpp
	@echo $(shell uname -m)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)