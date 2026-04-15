CC=/opt/cegcc/bin/arm-mingw32ce-gcc
LD=/opt/cegcc/bin/arm-mingw32ce-gcc

CFLAGS=-O2 -march=armv5tej -mcpu=arm926ej-s -Wno-attributes -fno-pic -DUNICODE -DMETRICS_EXPORTS
LDFLAGS=-O2 -march=armv5tej -mcpu=arm926ej-s -Wno-attributes -fno-pic -s

DLL=metrics.dll

.PHONY: build clean

build: $(DLL)

clean:
	rm $(DLL) *.o

$(DLL): metrics.o
	$(LD) -shared -o $@ $< $(LDFLAGS)
