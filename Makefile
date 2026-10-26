CXX64 ?= x86_64-w64-mingw32-g++
CXX32 ?= i686-w64-mingw32-g++
CC64  ?= x86_64-w64-mingw32-gcc
CC32  ?= i686-w64-mingw32-gcc
WINDRES64 ?= x86_64-w64-mingw32-windres
WINDRES32 ?= i686-w64-mingw32-windres

CXX_LINUX ?= g++
CC_LINUX  ?= gcc

CXXFLAGS = -std=c++17 -O2 -Wall -Wno-unused-variable -DUNICODE -D_UNICODE -mwindows -static -static-libgcc -static-libstdc++
CFLAGS   = -O2 -Wall -DUNICODE -D_UNICODE
LDFLAGS  = -lgdiplus -lgdi32 -lcomctl32 -lshlwapi -lole32 -luxtheme -lmsimg32 -lwinmm

GTK_CFLAGS  = $(shell pkg-config --cflags gtk+-3.0 cairo pangocairo)
GTK_LDFLAGS = $(shell pkg-config --libs gtk+-3.0 cairo pangocairo) -lpthread

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

SRCS_LINUX_CPP = src/main_linux.cpp \
                 src/pet_renderer_cairo.cpp \
                 src/pet_overlay_linux.cpp \
                 src/settings_dialog_linux.cpp \
                 src/settings_manager.cpp \
                 src/skin_store.cpp

OBJS64_CPP = $(patsubst src/%.cpp, build64/%.o, $(SRCS_CPP))
OBJS64_C   = $(patsubst src/%.c, build64/%.o, $(SRCS_C))
OBJS64_RC  = build64/resource.o

OBJS32_CPP = $(patsubst src/%.cpp, build32/%.o, $(SRCS_CPP))
OBJS32_C   = $(patsubst src/%.c, build32/%.o, $(SRCS_C))
OBJS32_RC  = build32/resource.o

TARGET64 = bin/GUPet_x64.exe
TARGET32 = bin/GUPet_x86.exe
TARGET_LINUX = bin/gupet

all: x64 x86 linux

x64: $(TARGET64)
x86: $(TARGET32)
linux: $(TARGET_LINUX)

$(TARGET64): $(OBJS64_CPP) $(OBJS64_C) $(OBJS64_RC) | bin
	$(CXX64) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	x86_64-w64-mingw32-strip -s $@

$(TARGET32): $(OBJS32_CPP) $(OBJS32_C) $(OBJS32_RC) | bin
	$(CXX32) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	i686-w64-mingw32-strip -s $@

$(TARGET_LINUX): $(SRCS_LINUX_CPP) $(SRCS_C) | bin
	$(CXX_LINUX) -std=c++17 -O2 -Wall -Wno-unused-variable $(GTK_CFLAGS) -o $@ $^ $(GTK_LDFLAGS)
	strip -s $@

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

.PHONY: all x64 x86 linux clean
