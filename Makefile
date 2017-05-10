thisdir := jay
SUBDIRS := 

CCOMPILE = g++  

LOCAL_CFLAGS = -DSKEL_DIRECTORY=\""$(prefix)/share/jay"\"

sources = Closure.cpp ErrorReporter.cpp GlobalInfo.cpp LALR.cpp LR0.cpp main.cpp ParserGenerator.cpp Printer.cpp Reader.cpp SymbolTable.cpp Verbose.cpp Warshall.cpp

datafiles = ACKNOWLEDGEMENTS NEW_FEATURES NOTES README README.jay skeleton \
            skeleton.cs

DISTFILES = $(datafiles) $(sources) jay.1 $(wildcard *.h) jay.vcxproj

all-local: jay

install-local: jay
uninstall-local:

ifndef NO_INSTALL
install-local:
	$(MKINSTALLDIRS) $(DESTDIR)$(prefix)/bin
	$(MKINSTALLDIRS) $(DESTDIR)$(prefix)/share/jay
	$(MKINSTALLDIRS) $(DESTDIR)$(prefix)/share/man/man1
	$(INSTALL_BIN) jay $(DESTDIR)$(prefix)/bin
	for datafile in $(datafiles) ; do \
	   $(INSTALL_DATA) $$datafile $(DESTDIR)$(prefix)/share/jay ; \
	done
	$(INSTALL_DATA) jay.1 $(DESTDIR)$(prefix)/share/man/man1

uninstall-local:
	-rm -f $(DESTDIR)$(prefix)/bin/jay
	for datafile in $(datafiles) ; do \
	   rm -f $(DESTDIR)$(prefix)/share/jay/$$datafile || : ; \
	done
	-rm -f $(DESTDIR)$(prefix)/share/man/man1/jay.1
endif

csproj-local:

clean-local:
	rm -f jay *.o *.exe *.pdb

dist-local: dist-default

test-local run-test-local run-test-ondotnet-local doc-update-local:

jay: $(sources:.cpp=.o)
	$(CCOMPILE) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CCOMPILE) $(JAY_CFLAGS) -c -o $@ $^
