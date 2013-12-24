CC=g++

EXE=tsstore

SRCDIR=src
BINDIR=bin
OBJDIR=obj

TEST_SRCDIR=testsrc
TEST_BINDIR=testbin

GTESTDIR=/home/ubuntu/lib/gtest-1.7.0

SRCS=$(wildcard $(SRCDIR)/*.cc)
OBJS=$(SRCS:$(SRCDIR)/%.cc=$(OBJDIR)/%.o)

TEST_SRCS=$(wildcard $(TEST_SRCDIR)/*.cc)
TEST_OUTS=$(TEST_SRCS:$(TEST_SRCDIR)/%.cc=$(TEST_BINDIR)/%)

all: $(BINDIR)/$(EXE)


test: $(TEST_OUTS)
	echo $<
	for test in '$<'; do $$test; done

$(TEST_BINDIR)/%: $(TEST_SRCDIR)/%.cc $(OBJDIR)/libgtest.a | $(TEST_BINDIR)
	$(CC) -isystem $(GTESTDIR)/include -pthread $< $(OBJDIR)/libgtest.a -o $@

#
# GTEST
#

$(OBJDIR)/gtest-all.o: | $(OBJDIR)
	$(CC) -isystem $(GTESTDIR)/include -I$(GTESTDIR) -L$(GTESTDIR)/lib -pthread -c $(GTESTDIR)/src/gtest-all.cc -o $(OBJDIR)/gtest-all.o

$(OBJDIR)/libgtest.a: $(OBJDIR)/gtest-all.o
	ar -rv $(OBJDIR)/libgtest.a $(OBJDIR)/gtest-all.o




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

$(TEST_BINDIR):
	mkdir $(TEST_BINDIR)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
	rm -rf $(TEST_BINDIR)
