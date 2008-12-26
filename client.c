#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "templates.h"

ssize_t testwrite(void *arg, const void *buf, size_t count) {
	return write(1, buf, count);
}

int main(int argc, char *argv[]) {
	struct exec *e;
	struct exec_run *r;
	int ret;
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;

	if (argc != 2) {
		fprintf(stderr, "\nsyntax: %s <file>\n\n", argv[0]);
		exit(1);
	}

	/* init side */

	e = exec_new_template();
	if (e == NULL) {
		fprintf(stderr, "file \"%s\": %s\n", argv[1], e->error);
		exit(1);
	}

	exec_set_write(e, testwrite);
	exec_set_easy(e, NULL);

	ret = exec_parse(e, argv[1]);
	if (ret != 0) {
		fprintf(stderr, "file \"%s\": %s\n", argv[1], e->error);
		exit(1);
	}

	ret = exec_display(e, "a", 0);
	if (ret != 0) {
		fprintf(stderr, "file \"%s\": %s\n", argv[1], e->error);
		exit(1);
	}

	/* run side */  

	gettimeofday(&tv1, NULL);

	r = exec_new_run(e);
	if (r == NULL) {
		fprintf(stderr, "file \"%s\": %s\n", argv[1], e->error);
		exit(1);
	}

	while (1) {
		ret = exec_run_now(r);
		if (ret == 1)
			continue;
		else if (ret == 0)
			break;
		else {
			fprintf(stderr, "file \"%s\": %s\n", argv[1], e->error);
			exit(1);
		}
	}

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
