/**
 * @file
 * @author philpem
 * 
 * CFR_LUT2CSV: Convert a CFR Filmtable LUT into a CSV file or Gnuplot DAT file.
 *
 * Reads in a Polaroid CFR (HR-6000 and possibly CI-5000S) film table and
 * outputs a CSV or DAT file containing the LUT data.
 */

#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common/filmtable.h"
#include "common/hexdump.h"

/**
 * @brief Display usage (CLI syntax) information
 */
void usage(const char *progname)
{
	fprintf(stderr, "Syntax: %s [-d|-e] [-i infile] [-o outfile]\n", progname);
	fprintf(stderr, "\t-d: input filmtable is not encrypted\n");
	fprintf(stderr, "\t-e: input filmtable is encrypted (default)\n");
	fprintf(stderr, "\t-i: set input file\n");
	fprintf(stderr, "Output is sent to stdout.\n");
}

int main(int argc, char **argv)
{
	bool encrypted = true;
	char infn[512] = "";
	FILE *fi;
	unsigned char *filmtable;
	int opt;

	while ((opt = getopt(argc, argv, "edi:o:")) != -1) {
		switch (opt) {
			case 'e':
				// Encrypted film table
				encrypted = true;
				break;
			case 'd':
				// Decrypted film table
				encrypted = false;
				break;
			case 'i':
				// Input filename
				if (strlen(optarg) > (sizeof(infn)-1)) {
					fprintf(stderr, "Error: input file name is too long!\n");
					return -1;
				}
				// I reserve the right to be a complete pessimist.
				strncpy(infn, optarg, sizeof(infn));
				infn[sizeof(infn)-1] = '\0';
				break;
/*			case 'o':
				// Output filename
				if (strlen(optarg) > (sizeof(outfn)-1)) {
					fprintf(stderr, "Error: output file name is too long!\n");
					return -1;
				}
				// I reserve the right to be a complete pessimist.
				strncpy(outfn, optarg, sizeof(outfn));
				infn[sizeof(outfn)-1] = '\0';
				break;
*/			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	// Check that all the params we need have been set
	if ((strlen(infn) == 0)) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// And now we begin.
	if ((fi = fopen(infn, "rb")) == NULL) {
		fprintf(stderr, "Error opening input file\n");
		return EXIT_FAILURE;
	}

	// Find out how long the filmtable data is
	size_t filmtab_len;
	fseek(fi, 0, SEEK_END);
	filmtab_len = ftell(fi);
	fseek(fi, 0, SEEK_SET);

	if (filmtab_len == 0) {
		fclose(fi);
		fprintf(stderr, "Error: this file is too small to be a CFR film table!\n");
		return EXIT_FAILURE;
	}

	// Allocate memory for the film table
	filmtable = (unsigned char *) malloc(filmtab_len);
	if (filmtable == NULL) {
		fclose(fi);
		fprintf(stderr, "Error reading filmtable file\n");
		return EXIT_FAILURE;
	}

	// Read the film table
	if (fread(filmtable, 1, filmtab_len, fi) != filmtab_len) {
		free(filmtable);
		fclose(fi);
		fprintf(stderr, "Error reading filmtable file\n");
		return EXIT_FAILURE;
	}

	fclose(fi);

	// Decrypt the film table if necessary
	if (encrypted) {
		size_t i;
		filmtable_crypto_init();
		for (i=0; i<filmtab_len; i++)
			filmtable[i] = filmtable_crypto_decrypt(filmtable[i]);
	}

	// Output the film name
	printf("# %s: %-24s\n", infn, (char *)filmtable);

	// Now decode the delta data
	unsigned int k, r, g, b;
	size_t i;
	k=r=b=g=0;
	for (i=108; i<filmtab_len; i+=4) {
		k += filmtable[i+3];
		r += filmtable[i+2];
		g += filmtable[i+1];
		b += filmtable[i+0];
		printf("%zu,%u,%u,%u,%u\n", (i-108)/4, r, g, b, k);
	}

	free(filmtable);

	return 0;
}
