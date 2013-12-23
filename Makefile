CC=g++

SRCDIR=src
BINDIR=bin
OBJDIR=obj

SRCS=$(wildcard $(SRCDIR)/*.cc)
OBJS=$(SRCS:$(SRCDIR)/%.cc=$(OBJDIR)/%.o)


all: tsstore

run: tsstore
	$(BINDIR)/tsstore

tsstore: $(OBJS) | $(BINDIR)
	$(CC) $(OBJS) -o $(BINDIR)/tsstore

$(OBJDIR)/%.o: $(SRCDIR)/%.cc | $(OBJDIR)
	$(CC) -c -o $@ $<

$(OBJDIR):
	mkdir $(OBJDIR)

$(BINDIR):
	mkdir $(BINDIR)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
