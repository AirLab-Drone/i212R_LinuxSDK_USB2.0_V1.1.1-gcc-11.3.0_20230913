CC=g++
TARGET=build//rasp_dji_payload
ifeq ($(shell uname -m), x86_64)
	CFLAGS=-I./include -I/usr/local/include/opencv4
	LDFLAGS=-L./lib/x64 -lGuideUSB2LiveStream -lpthread -lm -ludev -lopencv_core -lopencv_highgui -lopencv_imgproc -lmemcached
else
	CFLAGS=-I./include
	LDFLAGS=-L./lib/arrch -lGuideUSB2LiveStream -lpthread -lm -lmemcached
endif
all: $(TARGET)

$(TARGET): rasp_dji_payload.cpp
	@echo $(shell uname -m)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)