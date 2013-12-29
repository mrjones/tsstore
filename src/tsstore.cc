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




TSStore::TSStore(const Options& options,
                 BlockDevice* device) :
  options_(options), device_(device), next_id_(0) { }

TSStore::~TSStore() { }

TSStore::Block TSStore::AllocateBlock() {
  Block b;
  b.offset = 0;
  b.length = options_.block_size;
  return b;
}

TSWriter::TSWriter(TSID series_id) : series_id_(series_id) { }

bool TSWriter::Write(int64_t timestamp, std::vector<int64_t> data) {
  return false;
}

TSReader::TSReader(TSID series_id) : series_id_(series_id) { }

bool TSReader::Next(int64_t* timestamp_out, std::vector<int64_t>* data_out) {
  return false;
}
