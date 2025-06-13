.POSIX:

CC            = c++
CFLAGS        = -std=c++17 -Wall -Wextra -Wpedantic -g -DTARGET=\"$(TARGET)\"
LDFLAGS       = 

TARGET        = ed++
PREFIX        = /usr/local
BINDIR        = $(PREFIX)/bin
MANDIR        = $(PREFIX)/share/man
SRCDIR        = src
BUILDDIR      = build

$(BUILDDIR)/$(TARGET): $(BUILDDIR)/ed++.o
	$(CC) $(CFLAGS) $(BUILDDIR)/ed++.o -o $@ $(LDFLAGS)

$(BUILDDIR)/ed++.o: $(SRCDIR)/ed++.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)/ed++.cpp -o $@

clean:
	rm -f $(BUILDDIR)/*

install: $(TARGET)
	cp $(BUILDDIR)/$(TARGET) $(BINDIR)/$(TARGET)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

tests: $(BUILDDIR)/$(TARGET)
	TARGET=$(BUILDDIR)/$(TARGET) testsh --test
