CXX64 ?= x86_64-w64-mingw32-g++
CXX32 ?= i686-w64-mingw32-g++
CC64  ?= x86_64-w64-mingw32-gcc
CC32  ?= i686-w64-mingw32-gcc
WINDRES64 ?= x86_64-w64-mingw32-windres
WINDRES32 ?= i686-w64-mingw32-windres

CXXFLAGS = -std=c++17 -O2 -Wall -Wno-unused-variable -DUNICODE -D_UNICODE -mwindows -static -static-libgcc -static-libstdc++
CFLAGS   = -O2 -Wall -DUNICODE -D_UNICODE
LDFLAGS  = -lgdiplus -lgdi32 -lcomctl32 -lshlwapi -lole32 -luxtheme -lmsimg32 -lwinmm

SRCS_CPP = src/main.cpp \
           src/pet_renderer.cpp \
           src/skin_store.cpp \
           src/audio_monitor.cpp \
           src/settings_manager.cpp \
           src/speech_bubble.cpp \
           src/poop_manager.cpp \
           src/pet_overlay.cpp \
           src/settings_dialog.cpp \
           src/tray_icon.cpp

SRCS_C = src/miniz.c

OBJS64_CPP = $(patsubst src/%.cpp, build64/%.o, $(SRCS_CPP))
OBJS64_C   = $(patsubst src/%.c, build64/%.o, $(SRCS_C))
OBJS64_RC  = build64/resource.o

OBJS32_CPP = $(patsubst src/%.cpp, build32/%.o, $(SRCS_CPP))
OBJS32_C   = $(patsubst src/%.c, build32/%.o, $(SRCS_C))
OBJS32_RC  = build32/resource.o

TARGET64 = bin/GUPet_x64.exe
TARGET32 = bin/GUPet_x86.exe

all: x64 x86

x64: $(TARGET64)
x86: $(TARGET32)

$(TARGET64): $(OBJS64_CPP) $(OBJS64_C) $(OBJS64_RC) | bin
	$(CXX64) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	x86_64-w64-mingw32-strip -s $@

$(TARGET32): $(OBJS32_CPP) $(OBJS32_C) $(OBJS32_RC) | bin
	$(CXX32) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	i686-w64-mingw32-strip -s $@

build64/%.o: src/%.cpp | build64
	$(CXX64) $(CXXFLAGS) -c $< -o $@

build64/%.o: src/%.c | build64
	$(CC64) $(CFLAGS) -c $< -o $@

build64/resource.o: res/resource.rc res/resource.h res/gupet.ico res/gupet.manifest | build64
	$(WINDRES64) -Ires -I. $< -o $@

build32/%.o: src/%.cpp | build32
	$(CXX32) $(CXXFLAGS) -c $< -o $@

build32/%.o: src/%.c | build32
	$(CC32) $(CFLAGS) -c $< -o $@

build32/resource.o: res/resource.rc res/resource.h res/gupet.ico res/gupet.manifest | build32
	$(WINDRES32) -Ires -I. $< -o $@

bin build64 build32:
	mkdir -p $@

clean:
	rm -rf build64 build32 bin

.PHONY: all x64 x86 clean
