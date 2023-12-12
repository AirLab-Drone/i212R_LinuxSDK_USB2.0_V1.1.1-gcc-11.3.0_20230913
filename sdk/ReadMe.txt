gcc -I./include  Demo.c -L./lib -lGuideUSB2LiveStream -lpthread -lm -ludev -o Demo
./Demo


gcc -I./include /usr/local/include/opencv4 Demo.c -L./lib -lGuideUSB2LiveStream -lpthread -lm -ludev -o Demo
