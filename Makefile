TARGET = target 

$(warning "**************************************************************")
$(warning "                    compiler：g++ , ubuntu                    ")
$(warning "**************************************************************")
CC = gcc
CXX = g++

CPP_SRC := $(wildcard *.cpp src/*.cpp src/*/*.cpp)
CPP_OBJ := $(patsubst %.cpp, %.o, $(CPP_SRC))

INC_CXX := -I$$(pwd)/ThirdParty/ffmpeg/include \
		   -I$$(pwd)/src

# LIB_PATH := -L$$(pwd)/ThirdParty/ffmpeg/lib

LIB_CXX := -lavcodec -lavdevice -lavformat -lavutil -lavfilter \
		   -lpostproc -lswresample -lswscale -lx264

FLAGS = -O2 -Wall

all:$(TARGET)
$(TARGET):$(CPP_OBJ)
	$(CXX) $^ -o $@ $(LIB_PATH) $(LIB_CXX) $(FLAGS)
	# @rm $(CPP_OBJ) -f

%.o:%.cpp
	$(CXX) -c $< -o $@ $(INC_CXX)

.PHONY:clean
clean:
	-rm  -f $(CPP_OBJ) $(TARGET)
