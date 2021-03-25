TARGET = libicd.so

C_SRC = JPEG12.cpp JPEG8.cpp JPEG_codec.cpp LERC_codec.cpp PNG_codec.cpp \
	Packer_RLE.cpp icd_codecs.cpp

HEADERS = 

OBJECTS = $(C_SRC:.cpp=.o) libjpeg12.o Lerc1Image.o

CXXFLAGS = -fPIC -O2 -Wall

DEFINES = -DLINUX -D_REENTRANT -D_GNU_SOURCE $(DEBUG)

FILES = $(C_SRC) $(HEADERS)

LIBS = -ljpeg -lpng -lz

include ./Makefile.lcl

default	: $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CXX) -zdefs -shared -o $@ $(OBJECTS) $(LIBS)

%.o : %.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) $(DEFINES) -I . $(INCLUDES) -c $< -o $@

libjpeg12.o	:	jpeg12-6b/libjpeg12.o
	$(CP) $^ $@

jpeg12-6b/libjpeg12.o :
	$(MAKE) -C jpeg12-6b

Lerc1Image.o	:	lerc1/Lerc1Image.o
	$(CP) $^ $@

lerc1/LercImage.o	:
	$(MAKE) -C lerc1

clean:
	$(RM) -rf $(TARGET) $(OBJECTS)
	$(MAKE) -C jpeg12-6b clean
	$(MAKE) -C lerc1 clean


