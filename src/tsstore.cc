#include <iostream>

#include <inttypes.h>
#include <string.h>

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
  options_(options), device_(device), next_id_(0) {
}

TSStore::~TSStore() { }

std::unique_ptr<TSWriter> TSStore::OpenWriter(const std::string& name) {
  auto i = series_ids_.find(name);
  if (i == series_ids_.end()) {
    std::cout << "Error: Couldn't open: " << name << std::endl;
    return nullptr;
  }

  // shared ptr?
  Cursor c;
  c.block = AllocateBlock();
  c.block_offset = 0;
  return std::unique_ptr<TSWriter>(new TSWriter(i->second, c, device_.get()));
}

std::unique_ptr<TSReader> TSStore::OpenReader(const std::string& name) {
  auto i = series_ids_.find(name);
  if (i == series_ids_.end()) {
    std::cout << "Error: Couldn't open: " << name << std::endl;
    return nullptr;
  }

  Block dummy;
  dummy.offset = 0;
  dummy.length = options_.block_size;

  return std::unique_ptr<TSReader>(new TSReader(i->second, dummy, device_.get()));
}

Block TSStore::AllocateBlock() {
  Block b;
  b.offset = 0;
  b.length = options_.block_size;
  return b;
}

TSWriter::TSWriter(TSID series_id, Cursor cursor, BlockDevice* block_device)
  : series_id_(series_id), cursor_(cursor), block_device_(block_device) {
}

bool TSWriter::Write(int64_t timestamp, std::vector<int64_t> data) {
  int write_size = sizeof(int64_t) * (1 + data.size());
  if (write_size + cursor_.block_offset > cursor_.block.length) {
    // allocate new block
    std::cout << "Error: Can't allocate new blocks yet.";
  }

  char buf[write_size];

  memcpy(buf, &timestamp, sizeof(int64_t));

  for (uint i = 0; i < data.size(); ++i) {
    memcpy(buf + (i + 1) * sizeof(int64_t), &(data[i]), sizeof(int64_t));
  }

  int bytes_written = block_device_->Write(cursor_.block_offset, write_size, (void*)buf);
  cursor_.block_offset += bytes_written;
  return bytes_written == write_size;
}

TSReader::TSReader(TSID series_id, Block block, BlockDevice* block_device)
  : series_id_(series_id), block_device_(block_device) {
  cursor_.block = block;
  cursor_.block_offset = 0;
}

bool TSReader::Next(int64_t* timestamp_out, std::vector<int64_t>* data_out) {
  const int kColumns = 1;
  const int kReadSize = (1 + kColumns) * sizeof(int64_t);
  char buf[kReadSize];

  int bytes_read = block_device_->Read(cursor_.block.offset + cursor_.block_offset, kReadSize, &buf);

  if (bytes_read != kReadSize) {
    std::cout << "Error: Short read.";
    return false;
  }
  cursor_.block_offset += bytes_read;

  memcpy(timestamp_out, buf, sizeof(int64_t));

  data_out->resize(kColumns);
  for (int i = 0; i < kColumns; ++i) {
    memcpy(&((*data_out)[i]), buf + (1 + i) * sizeof(int64_t), sizeof(int64_t));
  }

  return true;
}
