#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include "common/filmtable.h"

typedef enum {
	CT_PACKFILM	= 0,		// Instant "pack" film
	CT_35MM		= 1,		// 35mm
	CT_AUTOFILM = 2,		// Instant "auto" (integral) film
	CT_4X5		= 3,		// 4x5
	CT_6X7		= 4,
	CT_6X8		= 5
} CFR_CAMERA_TYPE;

typedef struct {
	unsigned int		a, b, c;
} CFR_FT_GAIN;

typedef struct {
	unsigned int		a, b;
} CFR_FT_MAGIC;

// CFR film table header
typedef struct {
	char 				name[24];			// filmtable name
	CFR_CAMERA_TYPE		cameraType;			// camera type
	unsigned char		flags;				// film flags
	unsigned char		aspectWide, aspectTall;	// aspect ratio
	CFR_FT_GAIN			unknownA[8];		// unknown (1)	-- curiously this has 8 entries, same as UnknownB
	CFR_FT_MAGIC		unknownB[8];		// weird magic, unknown(3)
} CFR_FT_HEADER;

// CFR film table LUT entry
typedef struct {
	unsigned char		dR, dG, dB, dummy;
} CFR_FT_LUT_ENTRY;

// CFR film table
typedef struct {
	CFR_FT_HEADER		header;
	CFR_FT_LUT_ENTRY	lut[256];
} CFR_FILMTABLE;

int filmtable_read(FILE *fp, CFR_FILMTABLE *ft, bool encrypted)
{
	unsigned char *buf;
	size_t len, i;

	// get the length of the film table and check it
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (len != 1132)
		return -1;

	// allocate memory for the film table
	buf = malloc(len);
	if (buf == NULL)
		return -1;

	// read the file in
	if (fread(buf, 1, len, fp) != len) {
		free(buf);
		return -1;
	}

	// decrypt filmtable if necessary
	if (encrypted) {
		filmtable_crypto_init();
		for (i=0; i<len; i++) {
			buf[i] = filmtable_crypto_decrypt(buf[i]);
		}
	}

	// 108 byte header, 1024 byte lut

	// read in the header
	// table name
	for (i=0; i<24; i++)
		ft->header.name[i] = buf[i];
	ft->header.cameraType	= buf[24];
	ft->header.flags		= buf[25];
	ft->header.aspectWide	= buf[26];
	ft->header.aspectTall	= buf[27];

	// Current theory --
	//   There are eight sets of channel gain triads
	//   There are also eight pairs of width/<n> values.
	//   The width/<n> table data may be related to the channel gain triads.
	//
	//   Need to check, but W/N pairs appear to be the same for all films of a given type.
	//   Chances are the W/N pairs where W=0 may have the same index as the CGs with 

	// FIXME Unknown -- possibly colour channel gain/multiplier for the LUT
	// FIXME may also be related to UnknownB's value pairs
	for (i=0; i<8; i++) {
		ft->header.unknownA[i].a = buf[28+(i*6)] + (buf[29+(i*6)] << 8);	// little endian
		ft->header.unknownA[i].b = buf[30+(i*6)] + (buf[31+(i*6)] << 8);	// little endian
		ft->header.unknownA[i].c = buf[32+(i*6)] + (buf[33+(i*6)] << 8);	// little endian
	}

	// FIXME Unknown -- seems to be pairs of values, width and some form of correction factor
	for (i=0; i<8; i++) {
		ft->header.unknownB[i].a = buf[76+(i*4)] + (buf[77+(i*4)] << 8);	// little endian
		ft->header.unknownB[i].b = buf[78+(i*4)] + (buf[79+(i*4)] << 8);	// little endian
	}

	// read in the LUT
	for (i=0; i<256; i++) {
		// FIXME these are probably in the wrong order
		ft->lut[i].dR = buf[108+(i*4)+0];
		ft->lut[i].dG = buf[108+(i*4)+1];
		ft->lut[i].dB = buf[108+(i*4)+2];
		ft->lut[i].dummy = buf[108+(i*4)+3];
	}

	free(buf);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("syntax: %s filename...\n", argv[0]);
		return 1;
	}

	FILE *fp;
	int i, j;

	for (i=1; i<argc; i++) {
		CFR_FILMTABLE table;
		unsigned long lutmax[4];
		bool encrypted = false;

		if (strcasecmp(&argv[i][strlen(argv[i])-4], ".flm") == 0)
			encrypted = true;

		fp = fopen(argv[i], "rb");
		if (filmtable_read(fp, &table, encrypted) == 0) {
			printf("---> %s\n", argv[i]);
			printf("Name: %-24s\n", table.header.name);
			printf("CameraType: %d\n", table.header.cameraType);
			printf("Flags: %d\n", table.header.flags);
			printf("AspectRatio: %d %d\n", table.header.aspectWide, table.header.aspectTall);
			for (j=0; j<8; j++) {
				printf("GT ent %d: w=%4d, (%3d%s %3d%s %3d%s), %d\n",
						j+1,
						table.header.unknownB[j].a,
						table.header.unknownA[j].a & 0x7fff, (table.header.unknownA[j].a & 0x8000) ? "!" : " ",
						table.header.unknownA[j].b & 0x7fff, (table.header.unknownA[j].b & 0x8000) ? "!" : " ",
						table.header.unknownA[j].c & 0x7fff, (table.header.unknownA[j].c & 0x8000) ? "!" : " ",
						table.header.unknownB[j].b
					  );
			}

			// Calculate maximum value in each LUT
			memset(&lutmax, '\0', sizeof(lutmax));
			for (j=0; j<256; j++) {
				lutmax[0] += table.lut[j].dR;
				lutmax[1] += table.lut[j].dG;
				lutmax[2] += table.lut[j].dB;
				lutmax[3] += table.lut[j].dummy;
			}

			printf("LUTMax: %lu %lu %lu %lu\n", lutmax[0], lutmax[1], lutmax[2], lutmax[3]);
			printf("\n\n");
		}
	}

	return 0;
}
