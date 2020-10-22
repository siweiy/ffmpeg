TARGET = target 
TOOL = Tool/FlvTool

$(warning "**************************************************************")
$(warning "                    compiler：g++ , ubuntu                    ")
$(warning "**************************************************************")
CC = gcc
CXX = g++

CPP_SRC := $(wildcard *.cpp src/*.cpp src/*/*.cpp Sample/*.cpp)
CPP_OBJ := $(patsubst %.cpp, %.o, $(CPP_SRC))

INC_CXX := -I$$(pwd)/ThirdParty/ffmpeg/include \
		   -I$$(pwd)/ThirdParty/librtmp/include \
		   -I$$(pwd)/Sample \
		   -I$$(pwd)/src

# LIB_PATH := -L$$(pwd)/ThirdParty/ffmpeg/lib \
# 			-L$$(pwd)/ThirdParty/librtmp/lib \
# 			-L$$(pwd)/ThirdParty/x264/lib

LIB_PATH := -L$$(pwd)/ThirdParty/librtmp/lib


LIB_CXX := -lavcodec -lavdevice -lavformat -lavutil -lavfilter \
		   -lpostproc -lswresample -lswscale -lrtmp

FLAGS = -O2 -Wall
HEAD_FLAGS = -std=c++11 #-DDEBUG

all:$(TARGET)
$(TARGET):$(CPP_OBJ)
	$(CXX) $^ -o $@ $(LIB_PATH) $(LIB_CXX) $(FLAGS)
	@rm $(CPP_OBJ) -f

%.o:%.cpp
	$(CXX) -c $< -o $@ $(INC_CXX) $(HEAD_FLAGS)

.PHONY:clean
clean:
	-rm  -f $(CPP_OBJ) $(TARGET) $(TOOL)
