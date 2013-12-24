#include <iostream>
#include <inttypes.h>

const int MAX_NAME_LEN = 32;
const int MAX_COLS = 8;

struct Superblock {
  int64_t timestamp;
};

struct Column {
  enum Type {
    INT64,
  };

  Type type;
  char name[MAX_NAME_LEN];
};

struct SeriesMetadata {
  int64_t id;
  char name[MAX_NAME_LEN];
  int numCols;
  Column columns[MAX_COLS];
};

struct DataSegment {

};



int main(int argc, char* argv[]) {
  std::cout << "Hello world!" << std::endl;
}
