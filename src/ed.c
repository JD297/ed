#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct ed_state {
	char *pflag;
	int sflag;
	char *file;

	struct stat sb;

	char *buf;
	size_t nbuf;
	char *p_buf;

	char *cmd;
	size_t ncmd;
	char *ecmd;
} ed_state;

ed_state ed_init();

void ed_open(ed_state *ed);

void print_usage()
{
	fprintf(stderr, "Usage: %s [-] [-s] [-p string] [file]\n", TARGET);
	fprintf(stderr, "Line-oriented text editor\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "JD297 %s source code <https://github.com/jd297/ed>\n", TARGET);
}

int main(int argc, char *argv[])
{
	ed_state ed = ed_init();

	int opt;

	while ((opt = getopt(argc, argv, "sp:")) != -1) {
		switch (opt) {
			case 'p':
				ed.pflag = optarg;
				break;
			case 's':
				ed.sflag = 1;
				break;
			default:
				print_usage();
				exit(EXIT_FAILURE);
		}
	}

	if (optind < argc) {
		ed.file = argv[optind];
	}

	ed_open(&ed);

	if (ed.sflag != 1 && ed.file != NULL) {
		printf("%ld\n", ed.sb.st_size);
	}

	do {
		if (ed.cmd == NULL) {
			if ((ed.cmd = (char *)malloc(ed.ncmd)) == NULL) {
				err(EXIT_FAILURE, "malloc");
			}
		}

		memset(ed.cmd, 0, ed.ncmd);

		if (ed.pflag) {
			printf("%s", ed.pflag);
		}

		char buffer[1];
		size_t nread_total;

		for (nread_total = 0; ; nread_total++) {
			ssize_t nread;

			if ((nread = read(STDIN_FILENO, buffer, 1)) == -1) {
				err(EXIT_FAILURE, "read: cmd");
			}

			if (nread == 0) {
				break;
			}

			if (strncmp(buffer, "\n", 1) == 0) {
				break;
			}

			if (nread_total >= ed.ncmd) {
				ed.ncmd *= 2;

				if ((ed.cmd = (char *)realloc(ed.cmd, ed.ncmd)) == NULL) {
					err(EXIT_FAILURE, "realloc");
				}
			}

			strcat(ed.cmd, buffer);
		}

		if (nread_total == 0) {
			ed.ecmd = "Invalid address";

			goto print_error;
		}

		if (strncmp(ed.cmd, "q", 1) == 0) {
			break;
		}

		ed.ecmd = "Unknown command";

		print_error: {
			printf("?\n");
		}
	} while (1);

	free(ed.buf);
	free(ed.cmd);

	return 0;
}

ed_state ed_init()
{
	ed_state state;

	state.pflag = NULL;
	state.sflag = 0;
	state.file = NULL;

	state.ncmd = 4;
	state.cmd = NULL;

	return state;
}

#define ED_RESERVED_PAGES_FACTOR 2

void ed_open(ed_state *ed)
{
	if (ed->file == NULL) {
		goto empty_buffer;
	}

	if (stat(ed->file, &ed->sb) == -1) {
		fprintf(stderr, "%s: %s\n", ed->file, strerror(errno));

		goto empty_buffer;
	}

	if ((ed->sb.st_mode & S_IFMT) == S_IFDIR) {
		fprintf(stderr, "%s: %s\n", ed->file, strerror(EISDIR));

		goto empty_buffer;
	}

	int fd;

	if ((fd = open(ed->file, O_RDONLY)) == -1) {
		fprintf(stderr, "%s: %s\n", ed->file, strerror(errno));

		goto empty_buffer;
	}

	ed->nbuf = ((ed->sb.st_size / sysconf(_SC_PAGESIZE)) + 1) * ED_RESERVED_PAGES_FACTOR * sysconf(_SC_PAGESIZE);

	ed->buf = (char *)mmap(NULL,
	                       ed->nbuf,
	                       PROT_READ | PROT_WRITE,
	                       MAP_ANONYMOUS | MAP_PRIVATE,
	                       -1,
	                       0);

	if (ed->buf == MAP_FAILED) {
		err(EXIT_FAILURE, "mmap: %s", ed->file);
	}

	if (read(fd, ed->buf, ed->sb.st_size) == -1) {
		err(EXIT_FAILURE, "read: %s", ed->file);
	}

	ed->p_buf = ed->buf;

	if (close(fd) == -1) {
		err(EXIT_FAILURE, "close: %s", ed->file);
	}

	return;

	empty_buffer: {
		ed->file = NULL;

		ed->nbuf = 1 * ED_RESERVED_PAGES_FACTOR * sysconf(_SC_PAGESIZE);

		ed->buf = (char *)mmap(NULL,
		                       ed->nbuf,
		                       PROT_READ | PROT_WRITE,
		                       MAP_ANONYMOUS | MAP_PRIVATE,
		                       -1,
		                       0);

		if (ed->buf == MAP_FAILED) {
			err(EXIT_FAILURE, "mmap: %s", "no file");
		}

		ed->p_buf = ed->buf;
	}
}
