#include <inttypes.h>

#include "tsstore.h"

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




TSStore::TSStore(BlockDevice* device) : device_(device) { }
TSStore::~TSStore() { }

std::string TSStore::foo() {
  return "foo";
}
