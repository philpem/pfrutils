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
	-Wstrict-prototypes

.PHONY: all clean

all: cfr_morph

cfr_morph:	common/filmtable_crypt.o cfr_morph.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

