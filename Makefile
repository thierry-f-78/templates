#bison -d calc.y
#mv calc.tab.h calc.h
#mv calc.tab.c calc.y.c
#flex calc.lex
#mv lex.yy.c calc.lex.c
#gcc -c calc.lex.c -o calc.lex.o
#gcc -c calc.y.c -o calc.y.o
#gcc -o calc calc.lex.o calc.y.o -ll -lm [éventuellement -lfl]

LIBNAME = templates

# get build version from the git tree in the form "lasttag-changes", and use "dev" if unknown
BUILDVER := $(shell ref=$$((git describe --tags) 2>/dev/null) && ref=$${ref%-g*} && echo "$${ref\#v}")

LDFLAGS = 
CFLAGS = -Wall -g -O2
YFLAGS = -v

OBJS =  template.o syntax.o exec.o exec_run.o exec_trace.o
FILES = template.h template.c syntax.c syntax.h

ifeq ($(DEBUG),1)
	CFLAGS += -DDEBUG
endif

#YACC_DEBUG = -t
#CPPFLAGS = -DDEBUGING -DYYDEBUG
CPPFLAGS =

%.c %.h: %.y
	$(YACC) -o $(<:.y=.c) --defines=$(<:.y=.h) $<

%.c %.h: %.l
	$(LEX) -o $(<:.l=.c) --header-file=$(<:.l=.h) $<

all: lib$(LIBNAME).a $(LIBNAME) client

client: lib$(LIBNAME).a client.c
	$(CC) -o client client.c lib$(LIBNAME).a

lib$(LIBNAME).a: $(OBJS)
	$(AR) -rcv $@ $^

$(LIBNAME): client.o lib$(LIBNAME).a
	$(CC) -o $@ $^

clean:
	rm -f $(OBJS) $(FILES) lib$(LIBNAME).a $(LIBNAME) client.o

dot:
	cat a | dot -Gsize="11.0,7.6" -Gpage="8.3,11.7" -Tps | ps2pdf - graph.pdf
	# cat a | dot -Gsize="7.6,11.0" -Gpage="8.3,11.7" -Tps | ps2pdf - graph.pdf

pack:
	rm -rf /tmp/$(LIBNAME)-$(BUILDVER) >/dev/null 2>&1; \
	git clone . /tmp/$(LIBNAME)-$(BUILDVER) && \
	tar --exclude .git -C /tmp/ -vzcf $(LIBNAME)-$(BUILDVER).tar.gz $(LIBNAME)-$(BUILDVER) && \
	rm -rf /tmp/$(LIBNAME)-$(BUILDVER) >/dev/null 2>&1; \


syntax.h:   syntax.l
template.h: template.y
syntax.c:   syntax.l templates.h exec_internals.h template.h
template.c: template.y exec_internals.h syntax.h templates.h
syntax.o:   syntax.c syntax.h
template.o: template.c template.h
exec.o:     exec.c templates.h exec_internals.h
exec_run.o: exec_run.c templates.h exec_internals.h
exec_trace.o: exec_trace.c templates.h

