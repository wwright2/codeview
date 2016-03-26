

#include <stdio.h>
#include <string.h>
#include <stdlib.h>



int main(int argc, char *argv[])
{
	int i, nmatch;
	FILE *f;

	if (argc < 2)
		eprintf("usage: grep regexp [file ...]");
	nmatch = 0;

	if (argc == 2) {
		if (grep(argv[1], stdin, NULL))
			nmatch++;
	} else {
		for (i = 2; i < argc; i++) {
			f = fopen(argv[i], "r");
			if (f == NULL) {
				weprintf("can't open %s:", argv[i]);
				continue;
			}
			if (grep(argv[1], f, argc>3 ? argv[i] : NULL) > 0)
				nmatch++;
			fclose(f);
		}
	}
	return nmatch == 0;
}



