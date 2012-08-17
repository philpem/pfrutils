CC=gcc
CFLAGS=-fmessage-length=0 \
	-ansi -pedantic -std=c99 \
	-Wall \
	-Wextra \
	-Wwrite-strings \
	-Winit-self \
	-Wcast-align \
	-Wcast-qual \
	-Wpointer-arith \
	-Wstrict-aliasing \
	-Wformat=2 \
	-Wmissing-include-dirs \
	-Wno-unused-parameter \
	-Wuninitialized \
	-Wold-style-definition \
	-Wstrict-prototypes \
	-Wstrict-overflow=5 \
	-Wshadow \
	-Wfloat-equal

.PHONY: all clean

all: cfr_morph cfr_lut2csv

clean:
	-rm -f cfr_morph cfr_morph.o
	-rm -f common/hexdump.o common/filmtable_crypt.o

cfr_morph:	common/filmtable_crypt.o cfr_morph.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

cfr_lut2csv:	common/filmtable_crypt.o cfr_lut2csv.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
