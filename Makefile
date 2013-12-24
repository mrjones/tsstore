CC=g++

SRCDIR=src
BINDIR=bin
OBJDIR=obj

TEST_SRCDIR=testsrc
TEST_BINDIR=testbin

GTESTDIR=/home/ubuntu/lib/gtest-1.7.0

all: $(BINDIR)/$(EXE)

test: $(TEST_BINDIR)/tsstore_test
		$(TEST_BINDIR)/tsstore_test



$(TEST_BINDIR)/tsstore_test: $(TEST_SRCDIR)/tsstore_test.cc $(OBJDIR)/libgtest.a $(OBJDIR)/tsstore.o | $(TEST_BINDIR)
	$(CC) -isystem $(GTESTDIR)/include -pthread $(OBJDIR)/tsstore.o $< $(OBJDIR)/libgtest.a -o $@

#
# GTEST
#

$(OBJDIR)/gtest-all.o: | $(OBJDIR)
	$(CC) -isystem $(GTESTDIR)/include -I$(GTESTDIR) -L$(GTESTDIR)/lib -pthread -c $(GTESTDIR)/src/gtest-all.cc -o $(OBJDIR)/gtest-all.o

$(OBJDIR)/libgtest.a: $(OBJDIR)/gtest-all.o
	ar -rv $(OBJDIR)/libgtest.a $(OBJDIR)/gtest-all.o




run: $(BINDIR)/tsstore
	$(BINDIR)/tsstore


$(BINDIR)/tsstore: $(OBJDIR)/tsstore.o $(OBJDIR)/tsstore_main.o | $(BINDIR)
	$(CC) $(OBJDIR)/tsstore.o $(OBJDIR)/tsstore_main.o -o $(BINDIR)/tsstore

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
