#bison -d calc.y
#mv calc.tab.h calc.h
#mv calc.tab.c calc.y.c
#flex calc.lex
#mv lex.yy.c calc.lex.c
#gcc -c calc.lex.c -o calc.lex.o
#gcc -c calc.y.c -o calc.y.o
#gcc -o calc calc.lex.o calc.y.o -ll -lm [éventuellement -lfl]

LDFLAGS = 
CFLAGS = -Wall -g -O0
YFLAGS = -v

OBJS =  template.o syntax.o exec.o exec_run.o exec_trace.o
FILES = template.h template.c syntax.c syntax.h

#YACC_DEBUG = -t
#CPPFLAGS = -DDEBUGING -DYYDEBUG
CPPFLAGS =

ifneq ($(V),1)

%.c: %.l
	@echo "[LEX ] $@"
	@$(RM) $@
	@$(LEX.l) --header-file=toto.h $< > $@
	@sed -i 's/"<stdout>"/"$@"/g' $@

%.h: %.l
	@echo "[LEX ] $@"
	@$(RM) $@
	@$(LEX.l) --header-file=$@ $< >/dev/null

%.c: %.y
	@echo "[YACC] $@"
	@$(YACC.y) $<
	@mv -f y.tab.c $@
	@sed -i 's/"y.tab.c"/"$@"/g' $@

%.h: %.y
	@echo "[YACC] $@"
	@$(YACC.y) -d $<
	@rm y.tab.c
	@mv y.tab.h $@

%.o: %.c
	@echo "[CC  ] $@"
	@$(COMPILE.c) $(OUTPUT_OPTION) $<

%: %.o
	@echo [LINK] $@
	@$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

else

%.c: %.y
	$(YACC.y) $<
	mv -f y.tab.c $@
	sed -i 's/"y.tab.c"/"$@"/g' $@

%.h: %.y
	$(YACC.y) -d $<
	rm y.tab.c
	mv y.tab.h $@

%.c: %.l
	$(RM) $@
	$(LEX.l) $< > $@
	sed -i 's/"<stdout>"/"$@"/g' $@

%.h: %.l
	$(RM) $@
	$(LEX.l) --header-file=$@ $< >/dev/null

endif

all: libtemplates.a

libtemplates.a: $(OBJS)
	@echo [  AR] $@
	@$(AR) -rcv $@ $^

syntax.o:   syntax.c template.h
template.o: template.c syntax.h

clean:
	rm -f $(OBJS) $(FILES) libtemplates.a

dot:
	cat a | dot -Gsize="7.6,11.0" -Gpage="8.3,11.7" -Tps | ps2pdf - graph.pdf

