/**
 * @file
 * @author philpem
 * 
 * CFR_Morph: Encrypt and decrypt Polaroid CFR film tables.
 *
 * Reads in a Polaroid CFR (HR-6000 and possibly CI-5000S) film table and
 * outputs a decrypted version. Can also encrypt a film table to produce
 * a .FLM file.
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
	fprintf(stderr, "\t-d: decrypt (default)\n");
	fprintf(stderr, "\t-e: encrypt\n");
	fprintf(stderr, "\t-i: set input file\n");
	fprintf(stderr, "\t-o: set output file\n");
}

int main(int argc, char **argv)
{
	bool encrypt = false;
	char infn[512] = "", outfn[512] = "";
	FILE *fi, *fo;
	int opt;

	while ((opt = getopt(argc, argv, "edi:o:")) != -1) {
		switch (opt) {
			case 'e':
				// Encrypt
				encrypt = true;
				break;
			case 'd':
				// Decrypt
				encrypt = false;
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
			case 'o':
				// Output filename
				if (strlen(optarg) > (sizeof(outfn)-1)) {
					fprintf(stderr, "Error: output file name is too long!\n");
					return -1;
				}
				// I reserve the right to be a complete pessimist.
				strncpy(outfn, optarg, sizeof(outfn));
				infn[sizeof(outfn)-1] = '\0';
				break;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	// Check that all the params we need have been set
	if ((strlen(infn) == 0) || (strlen(outfn) == 0)) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// And now we begin.
	if ((fi = fopen(infn, "rb")) == NULL) {
		fprintf(stderr, "Error opening input file\n");
		return EXIT_FAILURE;
	}
	if ((fo = fopen(outfn, "wb")) == NULL) {
		fclose(fi);
		fprintf(stderr, "Error opening output file\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%s film table %s to %s...\n", encrypt ? "Encrypting" : "Decrypting", infn, outfn);

	filmtable_crypto_init();
	while (!feof(fi)) {
		int c = fgetc(fi);
		if (c == EOF)
			break;

		if (encrypt) {
			c = filmtable_crypto_encrypt(c);
		} else {
			c = filmtable_crypto_decrypt(c);
		}

		if (fputc(c, fo) == EOF) {
			fprintf(stderr, "Error writing to output file :-(\n");
			fclose(fi);
			fclose(fo);
			unlink(outfn);
			return EXIT_FAILURE;
		}
	}

	fclose(fi);
	fclose(fo);

	return 0;
}
