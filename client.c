#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "templates.h"

ssize_t testwrite(void *arg, const void *buf, size_t count) {
	return write(1, buf, count);
}

void syntax(char *name) {
	printf("\n\nsyntax: %s [-s] [-d <dotfile>] <parsefile>\n"
	       "\t-s use with script\n"
	       "\t-d generate dotfile\n"
	       "\t<dotfile> the file when dot data are stored\n"
	       "\t<parsefile> the script file\n"
	       "\n", name);
	exit(1);
}

int main(int argc, char *argv[]) {
	struct exec *e;
	struct exec_run *r;
	int ret, ret2;
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;
	int i;
	int script = 0;
	char *filename = NULL;
	int dot = 0;
	char *dotname;
	FILE *fd;

	for (i=1; i<argc; i++) {
		if (argv[i][0] == '-')
			switch (argv[i][1]) {
			case 's':
				script = 1;
				break;

			case 'd':
				dot = 1;
				i++;
				if (i == argc)
					syntax(argv[0]);
				dotname = argv[i];
				break;

			default:
				syntax(argv[0]);

			}
		else
			filename = argv[i];
			
	}

	if (filename == NULL)
		syntax(argv[0]);

	/* init side */
	e = exec_new_template();
	if (e == NULL) {
		fprintf(stderr, "file \"%s\": %s\n", filename, e->error);
		exit(1);
	}

	exec_set_write(e, testwrite);
	exec_set_easy(e, NULL);

	/* open filename */
	fd = fopen(filename, "r");
	if (fd == NULL) {
		fprintf(stderr, "fopen(%s): %s\n", filename, strerror(errno));
		exit(1);
		return -1;
	}

	/* read first line */
	if (script == 1) {
		char buf[2048];
		fgets(buf, 2048, fd);
	}

	ret2 = exec_parse_file(e, fd);
	fclose(fd);

	/* build dot output */
	if (dot == 1) {
		ret = exec_display(e, dotname, 0);
		if (ret != 0) {
			fprintf(stderr, "file \"%s\": %s\n", filename, e->error);
			exit(1);
		}
	}

	/* errors */
	if (ret2 != 0) {
		fprintf(stderr, "file \"%s\": %s\n", filename, e->error);
		exit(1);
	}

	/* run side */  

	gettimeofday(&tv1, NULL);

	r = exec_new_run(e);
	if (r == NULL) {
		fprintf(stderr, "file \"%s\": %s\n", filename, e->error);
		exit(1);
	}

	while (1) {
		ret = exec_run_now(r);
		if (ret == 1)
			continue;
		else if (ret == 0)
			break;
		else {
			fprintf(stderr, "file \"%s\": %s\n", filename, e->error);
			exit(1);
		}
	}

	/* free exec run session */
	exec_clear_run(r);

	gettimeofday(&tv2, NULL);

	tv3.tv_sec = tv2.tv_sec - tv1.tv_sec;
	tv3.tv_usec = tv2.tv_usec - tv1.tv_usec;
	if (tv3.tv_usec < 0) {
		tv3.tv_sec--;
		tv3.tv_usec+=1000000;
	}

	fprintf(stderr, "exec time: %d.%d\n", (int)tv3.tv_sec, (int)tv3.tv_usec);

	return 0;
}
