#include <iostream>

#include <inttypes.h>
#include <string.h>

#include "tsstore.h"


TSStore::TSStore(const Options& options,
                 BlockDevice* device) :
  options_(options), device_(device), next_id_(0) {
}

TSStore::~TSStore() { }

std::unique_ptr<TSWriter> TSStore::OpenWriter(const std::string& name) {
  auto id = series_ids_.find(name);
  if (id == series_ids_.end()) {
    std::cout << "Error: Couldn't open: " << name << std::endl;
    return nullptr;
  }

  TSID tsid = id->second;
  auto series_info = series_.find(tsid);
  if (series_info == series_.end()) {
    std::cout << "Internal Error: Couldn't find info for id: " << tsid << std::endl;
  }

  // shared ptr?
  Cursor c;
  c.segment = AllocateSegment();
  c.segment_offset = 0;
  return std::unique_ptr<TSWriter>(new TSWriter(tsid, series_info->second.spec, c, device_.get()));
}

std::unique_ptr<TSReader> TSStore::OpenReader(const std::string& name) {
  auto id = series_ids_.find(name);
  if (id == series_ids_.end()) {
    std::cout << "Error: Couldn't open: " << name << std::endl;
    return nullptr;
  }

  TSID tsid = id->second;
  auto series_info = series_.find(tsid);
  if (series_info == series_.end()) {
    std::cout << "Internal Error: Couldn't find info for id: " << tsid << std::endl;
  }

  // TODO: actually locate the data segment
  Segment dummy;
  dummy.offset = 0;
  dummy.length = options_.segment_size;

  return std::unique_ptr<TSReader>(new TSReader(tsid, series_info->second.spec, dummy, device_.get()));
}

TSID TSStore::CreateSeries(const SeriesSpec& spec) {
  TSID id = next_id_++;

  Timeseries* timeseries = &(series_[id]);
  timeseries->spec = spec;
    
  series_ids_[spec.name] = id;

  return id;
}


Segment TSStore::AllocateSegment() {
  Segment b;
  b.offset = 0;
  b.length = options_.segment_size;
  return b;
}

TSWriter::TSWriter(TSID series_id, const SeriesSpec& spec, const Cursor& cursor, BlockDevice* block_device)
  : series_id_(series_id), spec_(spec), cursor_(cursor), block_device_(block_device) {
}

bool TSWriter::Write(int64_t timestamp, std::vector<int64_t> data) {
  int write_size = sizeof(int64_t) * (2 + data.size());
  if (write_size + cursor_.segment_offset > cursor_.segment.length) {
    // allocate new segment
    std::cout << "Error: Can't allocate new segments yet.";
  }

  char buf[write_size];

  memcpy(buf, &timestamp, sizeof(int64_t));
  for (uint i = 0; i < data.size(); ++i) {
    memcpy(buf + (i + 1) * sizeof(int64_t), &(data[i]), sizeof(int64_t));
  }
  // Writes a 0 timestamp for the next entry to terminate this timeseries.
  memset(buf + (data.size() + 1) * sizeof(int64_t), 0, sizeof(int64_t));

  int bytes_written = block_device_->Write(cursor_.segment_offset, write_size, (void*)buf);
  cursor_.segment_offset += bytes_written - sizeof(int64_t);  // subtract off the 0 terminator
  return bytes_written == write_size;
}

TSReader::TSReader(TSID series_id, const SeriesSpec& spec, const Segment& segment, BlockDevice* block_device)
  : series_id_(series_id), spec_(spec), block_device_(block_device) {
  cursor_.segment = segment;
  cursor_.segment_offset = 0;
}

bool TSReader::Next(int64_t* timestamp_out, std::vector<int64_t>* data_out) {
  // TODO: get the number of columns from the series spec
  const int kColumns = 1;
  const int kReadSize = (1 + kColumns) * sizeof(int64_t);
  char buf[kReadSize];

  int bytes_read = block_device_->Read(cursor_.segment.offset + cursor_.segment_offset, kReadSize, &buf);

  if (bytes_read != kReadSize) {
    std::cout << "Error: Short read.";
    return false;
  }
  cursor_.segment_offset += bytes_read;

  memcpy(timestamp_out, buf, sizeof(int64_t));

  if (*timestamp_out == 0) {
    // A timestamp of 0 indicates the end of the datastream.
    return false;
  }

  data_out->resize(kColumns);
  for (int i = 0; i < kColumns; ++i) {
    memcpy(&((*data_out)[i]), buf + (1 + i) * sizeof(int64_t), sizeof(int64_t));
  }

  return true;
}
