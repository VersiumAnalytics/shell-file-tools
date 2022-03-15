# Directories for object files and binaries
OBJDIR := obj
BINDIR := ../bin


# C/C++ compilers, options, and dependencies
CDEPS=vstrutils.h
CPPDEPS=$(CDEPS) vhash.h vmath.h
CC=gcc
CFLAGS=-I. -Wno-implicit-function-declaration
CPPFLAGS=$(CFLAGS) -Wno-c++11-extensions -std=c++14 -Wno-write-strings
CPPLINKER=-lstdc++


# Define all binaries produced and set default build as 'all'
BINS := $(addprefix $(BINDIR)/, fslicer fld-ctr get-fs rnd-extract fwc hashpend)
all: $(BINS) | $(BINDIR)


# Make a 'clean' command and define its behavior
.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/*


# Create object and bin directories if necessary
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)


# Recipes for creating object files from .c and .cpp files
$(OBJDIR)/%.o: %.c $(CDEPS) | $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJDIR)/%.o: %.cpp $(CPPDEPS) | $(OBJDIR)
	$(CC) -c -o $@ $< $(CPPFLAGS)


# Recipes for creating binaries from object files
$(BINDIR)/fld-ctr: $(OBJDIR)/fld-ctr.o $(OBJDIR)/vstrutils.o
	$(CC) $(OBJDIR)/fld-ctr.o $(OBJDIR)/vstrutils.o -o $@

$(BINDIR)/fslicer: $(OBJDIR)/fslicer.o
	$(CC) $(OBJDIR)/fslicer.o -o $@

$(BINDIR)/fwc: $(OBJDIR)/fwc.o $(OBJDIR)/vstrutils.o
	$(CC) $(OBJDIR)/fwc.o $(OBJDIR)/vstrutils.o -o $@

$(BINDIR)/get-fs: $(OBJDIR)/get-fs.o
	$(CC) $(OBJDIR)/get-fs.o $(CPPLINKER) -o $@

$(BINDIR)/rnd-extract: $(OBJDIR)/rnd-extract.o $(OBJDIR)/vstrutils.o
	$(CC) $(OBJDIR)/rnd-extract.o $(OBJDIR)/vstrutils.o -o $@

$(BINDIR)/hashpend: $(OBJDIR)/hashpend.o $(OBJDIR)/vmath.o $(OBJDIR)/vhash.o $(OBJDIR)/vstrutils.o
	$(CC) $(OBJDIR)/hashpend.o $(OBJDIR)/vhash.o $(OBJDIR)/vmath.o $(OBJDIR)/vstrutils.o $(CPPLINKER) -o $@
