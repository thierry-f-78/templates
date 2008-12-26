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
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;

	if (argc != 2) {
		fprintf(stderr, "\nsyntax: %s <file>\n\n", argv[0]);
		exit(1);
	}

	/* init side */

	e = exec_new_template();

	exec_set_write(e, testwrite);
	exec_set_easy(e, NULL);

	exec_parse(e, argv[1]);

	exec_display(e, "a", 0);

	/* run side */  

	gettimeofday(&tv1, NULL);

	r = exec_new_run(e);
	while (exec_run_now(r) != 0);

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
