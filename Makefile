PREFIX        = /usr/local
BINDIR        = $(PREFIX)/bin
MANDIR        = $(PREFIX)/share/man

TARGET        = ed++
TARGETDIR     = bin
BUILDDIR      = build
SRCDIR        = src
SRCINCLUDEDIR = include
TESTDIR       = tests

SRCFILEEXT    = cpp
SRCFILES      = $(wildcard $(SRCDIR)/*.$(SRCFILEEXT))
OBJFILEEXT    = o
OBJFILES      = $(patsubst $(SRCDIR)/%.$(SRCFILEEXT),$(BUILDDIR)/%.$(OBJFILEEXT),$(SRCFILES))


CC            = c++
CCLIBS        = -static
CCFLAGS       = -Wall -Wextra -Wpedantic -g
CCINCLUDE     = -I $(SRCINCLUDEDIR)
CCFLAGSPROG   = -DTARGET=\"$(TARGET)\"
CCFLAGSEXTRA  =

$(TARGETDIR)/$(TARGET): $(OBJFILES)
	$(CC) $(CCFLAGS) $(CCINCLUDE) $(OBJFILES) -o $(TARGETDIR)/$(TARGET) $(CCLIBS)

$(BUILDDIR)/%.$(OBJFILEEXT): $(SRCDIR)/%.$(SRCFILEEXT)
	$(CC) $(CCFLAGS) $(CCINCLUDE) $(CCFLAGSPROG) $(CCFLAGSEXTRA) -c -o $@ $<

clean:
	rm -f $(BUILDDIR)/*.$(OBJFILEEXT) $(TARGETDIR)/$(TARGET)

install: $(TARGET)
	cp $(TARGETDIR)/$(TARGET) $(BINDIR)/$(TARGET)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

.PHONY: tests-always-fail

tests: $(TARGETDIR)/$(TARGET) tests-always-fail
	TARGET=$(TARGETDIR)/$(TARGET) testsh --test
