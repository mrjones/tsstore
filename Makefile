CC=g++

EXE=tsstore

SRCDIR=src
BINDIR=bin
OBJDIR=obj

SRCS=$(wildcard $(SRCDIR)/*.cc)
OBJS=$(SRCS:$(SRCDIR)/%.cc=$(OBJDIR)/%.o)

all: $(BINDIR)/$(EXE)

run: $(BINDIR)/$(EXE)
	$(BINDIR)/$(EXE)

$(BINDIR)/$(EXE): $(OBJS) | $(BINDIR)
	$(CC) $(OBJS) -o $(BINDIR)/$(EXE)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc | $(OBJDIR)
	$(CC) -c $< -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

$(BINDIR):
	mkdir $(BINDIR)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
