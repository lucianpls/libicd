TARGET = libicd.so

C_SRC = JPEG12.cpp JPEG8.cpp JPEG_codec.cpp LERC_codec.cpp PNG_codec.cpp \
	Packer_RLE.cpp icd_codecs.cpp

HEADERS = 

OBJECTS = $(C_SRC:.cpp=.o) libjpeg12.o

CXXFLAGS = -fPIC -O2 -Wall

DEFINES = -DLINUX -D_REENTRANT -D_GNU_SOURCE $(DEBUG)

FILES = $(C_SRC) $(HEADERS)

include ./Makefile.lcl

default	:	$(OBJECTS)


%.o : %.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) $(DEFINES) -I . $(INCLUDES) -c $< -o $@

libjpeg12.o	:	jpeg12-6b/libjpeg12.o
	$(CP) $^ $@

jpeg12-6b/libjpeg12.o :
	$(MAKE) -C jpeg12-6b libjpeg12.o

clean:
	$(RM) -rf $(TARGET) $(OBJECTS)
	$(MAKE) -C jpeg12-6b clean


