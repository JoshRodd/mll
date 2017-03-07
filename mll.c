/* mll-0.2/mll.c */

static char szProgram[] = "mll";
static char szVersion[] = "0.2";

static char szCopyright[] = "\
Copyright (c) 2017, Josh Rodd\n\
All rights reserved.\n\
\n\
Redistribution and use in source and binary forms, with or without\n\
modification, are permitted provided that the following conditions are met:\n\
\n\
* Redistributions of source code must retain the above copyright notice, this\n\
  list of conditions and the following disclaimer.\n\
\n\
* Redistributions in binary form must reproduce the above copyright notice,\n\
  this list of conditions and the following disclaimer in the documentation\n\
  and/or other materials provided with the distribution.\n\
\n\
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n\
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n\
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n\
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n\
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n\
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n\
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n\
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n\
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n\
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\
";

static char szDescr[] = "\
Display maximum line length of stdin. Essentially, find the longest\n\
run of bytes between newline bytes. Only Unix format text files are\n\
supported, with lines terminated by a single newline. If the last\n\
line does not end in with a single newline byte, its length will\n\
not be considered. Input is always treated as ASCII or single-byte\n\
8-bit characters, not UTF-8 or anything else with wide characters.\n\
This program should be significantly faster than using GNU `wc -L'.\n\
\n\
If the --csv option is given, then the longest column instead of\n\
the longest line will be found. A column in a CSV file is delimited\n\
by the beginning of the line, a newline, a carriage return, or a\n\
comma. If the last line does not end with a newline or a carriage\n\
return, it will be ignored. If a column starts with a double quote,\n\
the delimiter will be considered to be a double quote followed by\n\
a newline, a carriage return, or a comma, provided that double quote\n\
was not preceded by a double quote. CSV files with newlines or\n\
carriage returns embedded within double quotes will be tolerated.\n\
Two consecutive double quotes within a double quoted string will\n\
be counted as two characters; the CSV mode reports the longest field\n\
in terms of bytes, not character.\n\
";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLOCKSIZE 128*1024*1024LL	/* 128MB block size */

typedef unsigned long long ull_t;

void version() {
	fprintf(stderr, "%s %s\n\
\n\
%s\n", szProgram, szVersion, szCopyright);
	exit(0);
}

void help(char *pszArgv0) {
	fprintf(stderr, "%s %s\n\nUsage: %s [--csv] [inputfile [...]]\n\n%s", szProgram, szVersion, pszArgv0, szDescr);
}

int mll(int fd, unsigned char *pbbuf, size_t cbbuf, char *psz, char *pszArgv0, int fCsv);

int main(int argc, char *argv[]) {
	int fd = 0;
	char *psz = NULL;
	char *pszArgv0 = argv[0];
	FILE *f;
	unsigned char *pbbuf;	/* The buffer we are using. */
	unsigned char *pbthis;	/* Where we are currently in the buffer. */
	size_t cbbuf = BLOCKSIZE; /* The size of the buffer we are using. */
	int rc;
	int fCsv = 0;

	int fEndOfOpts = 0;
	int iOption = 0;

	int rcMax = 0;

	if(!(pbbuf = malloc(cbbuf))) {
		perror("Out of memory");
		return 1;
	}

	while(argc > 1) {
		if(!strcmp(argv[1], "--")) fEndOfOpts = 1;
		else if(!strcmp(argv[1], "-")) { 
			if((rc = mll(0, pbbuf, cbbuf, NULL, pszArgv0, fCsv))) if(rc > rcMax) rcMax = rc;
			iOption++;		/* Just use stdin as the input file. */
		}
		else if(!strncmp(argv[1], "--", 2) && !fEndOfOpts) {
			if(!strcmp(argv[1] + 2, "version")) { version(); return 0; }
			else if(!strcmp(argv[1] + 2, "help")) { help(pszArgv0); return 0; }
			else if(!strcmp(argv[1] + 2, "csv")) { fCsv = 1; }
			else { help(pszArgv0); return 1; }
		}
		else if((argv[1][0] == '-') && !fEndOfOpts) {
			help(pszArgv0); return 1;
		}
		else {
			psz = argv[1];
			if(!(f = fopen(psz, "rb"))) {
				fprintf(stderr,"%s: %s: ", pszArgv0, psz);
				perror("Cannot open");
				if(1 > rcMax) rcMax = 1;
			} else {
				fd = fileno(f);
				if((rc = mll(fd, pbbuf, cbbuf, psz, pszArgv0, fCsv))) if(rc > rcMax) rcMax = rc;
				fclose(f);
			}
			iOption++;
		}
		argc--;	/* Advance to the next argument. */
		argv++;
	}

	/* Only default to doing stdin if no non-option arguments were passed in. */
	if(!iOption) {
		rc = mll(0, pbbuf, cbbuf, NULL, pszArgv0, fCsv);
		if(rc > rcMax) rcMax = rc;
	}

	return rcMax;
}

int mll(int fd, unsigned char *pbbuf, size_t cbbuf, char *psz, char *pszArgv0, int fCsv) {
	ull_t cbmax;		/* The longest line found so far. */
	ull_t cbleftover;
	ull_t cblastleftover;
	ull_t cbthis;		/* The number of bytes in the current buffer. */
	ull_t cbtotal;
	ssize_t cb;		/* The number of bytes read. */
	unsigned char *pbthis;	/* Where we are currently in the buffer. */
	cblastleftover = 0;
	cbleftover = 0;
	int fInDoubleQuote = 0;
	int fPriorCharWasDoubleQuote = 0;
	unsigned char b;
	int fContinue;

	cbmax = 0;
	do {
		cb = read(fd, pbbuf, cbbuf);
		cbthis = 0;
		if(cb > 0) {
			pbthis = pbbuf;
			while(cbthis < cb) {
				cbthis++;
				cbleftover++;
				b = *pbthis++;
				/* When not in CSV mode, all we look for is newlines. */
				if(!fCsv && (b != '\n')) continue;
				else if(fCsv) {
					fContinue = 1;

					/* In CSV mode, a carriage return means an end of field, provided we aren't inside a double quote. */
					if(b == '\r' && !fInDoubleQuote) fContinue = 0;

					/* Or a newline. */
					else if(b == '\n' && !fInDoubleQuote) fContinue = 0;

					/* Or a comma. */
					else if(b == ',' && !fInDoubleQuote) fContinue = 0;

					/* If we are in a double quote, another double quote means the end. */
					else if(fInDoubleQuote && b == '"') fInDoubleQuote = 0;

					/* Otherwise a double quote means the start. */
					else if(!fInDoubleQuote && b == '"') fInDoubleQuote = 1;

					if(fContinue) continue;

					fInDoubleQuote = 0;
					fPriorCharWasDoubleQuote = 0;
				}

				cbtotal = cbleftover + cblastleftover - 1;
				cblastleftover = 0;
				cbleftover = 0;
				if(cbtotal > cbmax) cbmax = cbtotal;
			}
			cblastleftover += cbleftover;
			cbleftover = 0;
		} else if(cb < 0) {
			fprintf(stderr, "%s: ", pszArgv0);
			if(psz) fprintf(stderr, "%s: ", psz);
			perror("Cannot read");
			return 1;
		}
	} while(cb > 0);

	/* When in CSV mode, the last line is not required to end with an end of line or end of field character. */
	if(fCsv) {
		cbtotal = cbleftover + cblastleftover;
		if(cbtotal > cbmax) cbmax = cbtotal;
	}

	if(!psz) 
		printf("%llu\n", cbmax);
	else {
		printf("%llu %s\n", cbmax, psz);
	}

	return 0;
}
