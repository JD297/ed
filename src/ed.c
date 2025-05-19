#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage()
{
	fprintf(stderr, "Usage: %s [-] [-s] [-p string] [file]\n", TARGET);
	fprintf(stderr, "Line-oriented text editor\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "JD297 %s source code <https://github.com/jd297/ed>\n", TARGET);
}

int main(int argc, char *argv[])
{
    char *pflag = NULL;
	int sflag = 0;
	char *file = NULL;

	int opt;

	while ((opt = getopt(argc, argv, "sp:")) != -1) {
		switch (opt) {
			case 'p':
				pflag = optarg;
				break;
			case 's':
				sflag = 1;
				break;
			default:
				print_usage();
				exit(EXIT_FAILURE);
		}
	}

    if (optind < argc) {
        file = argv[optind];
    }

	return 0;
}
