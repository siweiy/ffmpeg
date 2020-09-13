TARGET = target 

$(warning "**************************************************************")
$(warning "                    compiler：g++ , ubuntu                    ")
$(warning "**************************************************************")
CC = gcc
CXX = g++

CPP_SRC := $(wildcard *.cpp src/*.cpp src/*/*.cpp)
CPP_OBJ := $(patsubst %.cpp, %.o, $(CPP_SRC))

INC_CXX := -I$$(pwd)/ThirdParty/ffmpeg-4.3.1/include \
		   -I$$(pwd)/src

LIB_PATH := -L$$(pwd)/ThirdParty/ffmpeg-4.3.1/lib \
			-L$$(pwd)/ThirdParty/libx264/lib  \
			-L$$(pwd)/ThirdParty/libx265/lib

LIB_CXX := -lavcodec -lavdevice -lavformat -lavutil -lavfilter \
		   -lpostproc -lswresample -lswscale -lx264 -lx265

FLAGS = -O2 -Wall
HEAD_FLAGS = -DDEBUG

all:$(TARGET)
$(TARGET):$(CPP_OBJ)
	$(CXX) $^ -o $@ $(LIB_PATH) $(LIB_CXX) $(FLAGS)
	@rm $(CPP_OBJ) -f

%.o:%.cpp
	$(CXX) -c $< -o $@ $(INC_CXX) $(HEAD_FLAGS)

.PHONY:clean
clean:
	-rm  -f $(CPP_OBJ) $(TARGET)
