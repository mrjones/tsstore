#include <iostream>

#include <inttypes.h>
#include <string.h>

#include "tsstore.h"


SegmentLayoutManager::SegmentLayoutManager(int64_t device_size, int64_t segment_size)
  : /* DataSegmentAllocator(), */ device_size_(device_size), segment_size_(segment_size) {
  segments_.resize(device_size / segment_size);
}
 
SegmentLayoutManager::~SegmentLayoutManager() { }

bool SegmentLayoutManager::AllocateSeriesIndexSegment(TSID tsid, Segment* out) {
  for (uint i = 0; i < segments_.size(); ++i) {
    if (segments_[i].kind == SegmentInfo::EMPTY) {
      segments_[i].kind = SegmentInfo::SERIES_INDEX;
      segments_[i].tsid = tsid;

      out->offset = i * segment_size_;
      out->length = segment_size_;
      return true;
    }
  }

  std::cout << "Error No available segments." << std::endl;
  return false;
}

bool SegmentLayoutManager::AllocateDataSegment(TSID tsid, Timestamp first_timestamp, Segment* out) {
  for (uint i = 0; i < segments_.size(); ++i) {
    if (segments_[i].kind == SegmentInfo::EMPTY) {
      segments_[i].kind = SegmentInfo::SERIES_DATA;
      segments_[i].tsid = tsid;
      segments_[i].initial_timestamp = first_timestamp;

      out->offset = i * segment_size_;
      out->length = segment_size_;
      return true;
    }
  }
  std::cout << "Error: No available segments.";
  return false;
}


TSStore::TSStore(const Options& options,
                 BlockDevice* device) :
  options_(options),
  device_(device),
  next_id_(0),
  segment_layout_(device_->Size(), options_.segment_size) {
  std::cout << "New TSStore with segment size: " << options_.segment_size << std::endl;
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

  // TODO: get this from the index.
  // shared ptr?
  Cursor c;
  c.segment = AllocateSegment();
  c.segment_offset = 0;

  return std::unique_ptr<TSWriter>(new TSWriter(tsid, series_info->second.spec, c, device_.get(),  series_info->second.index.get()));
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

  return std::unique_ptr<TSReader>(new TSReader(tsid, series_info->second.spec, series_info->second.index.get(), device_.get()));
}

TSID TSStore::CreateSeries(const SeriesSpec& spec) {
  TSID id = next_id_++;

  Timeseries* timeseries = &(series_[id]);
  timeseries->spec = spec;

  Segment index_location;
  if (!segment_layout_.AllocateSeriesIndexSegment(id, &index_location)) {
    std::cout << "Error: Couldn't allocate index segment." << std::endl;
    return -1;
  }
  timeseries->index.reset(new SeriesIndex(id, index_location, &segment_layout_));

  series_ids_[spec.name] = id;

  std::cout << "Created new series. ID: " << id << " index location: " << index_location.offset << std::endl;
  return id;
}


Segment TSStore::AllocateSegment() {
  Segment b;
  b.offset = 0;
  b.length = options_.segment_size;
  return b;
}

TSWriter::TSWriter(TSID series_id,
                   const SeriesSpec& spec,
                   const Cursor& cursor,
                   BlockDevice* block_device,
                   SeriesIndex* index)
  : series_id_(series_id),
    spec_(spec),
    cursor_(cursor),
    block_device_(block_device),
    index_(index) {
}

namespace {
  int64_t Offset(const Cursor& c) {
    return c.segment.offset + c.segment_offset;
  }
}

bool TSWriter::Write(int64_t timestamp, std::vector<int64_t> data) {
  int write_size = sizeof(int64_t) * (2 + data.size());
  if (write_size + cursor_.segment_offset > cursor_.segment.length) {
    cursor_.valid = false;
  }

  if (!cursor_.valid) { 
    // allocate new segment
    if (!index_->AllocateSegment(timestamp, &(cursor_.segment))) {
      std::cout << "Error: Couldn't allocate new segment.";
      return false;
    } else {
      cursor_.valid = true;
      cursor_.segment_offset = 0;
    }
  }

  char buf[write_size];

  memcpy(buf, &timestamp, sizeof(int64_t));
  for (uint i = 0; i < data.size(); ++i) {
    memcpy(buf + (i + 1) * sizeof(int64_t), &(data[i]), sizeof(int64_t));
  }
  // Writes a 0 timestamp for the next entry to terminate this timeseries.
  memset(buf + (data.size() + 1) * sizeof(int64_t), 0, sizeof(int64_t));

  // TODO: helper
  int bytes_written = block_device_->Write(Offset(cursor_), write_size, (void*)buf);
  cursor_.segment_offset += bytes_written - sizeof(int64_t);  // subtract off the 0 terminator
  return bytes_written == write_size;
}



TSReader::TSReader(TSID series_id, const SeriesSpec& spec, SeriesIndex* index, BlockDevice* block_device)
  : series_id_(series_id), spec_(spec), index_(index), block_device_(block_device) {
  cursor_.valid = index->Next(-1, &timestamp_, &cursor_.segment);
  cursor_.segment_offset = 0;
}

bool TSReader::Next(int64_t* timestamp_out, std::vector<int64_t>* data_out) {
  // TODO: more data is added while iterating
  if (!cursor_.valid) {
    return false;
  }

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
    // A timestamp of 0 indicates the end of the segment
    cursor_.valid = index_->Next(timestamp_, &timestamp_, &cursor_.segment);
    cursor_.segment_offset = 0;

    return Next(timestamp_out, data_out);
  }

  data_out->resize(kColumns);
  for (int i = 0; i < kColumns; ++i) {
    memcpy(&((*data_out)[i]), buf + (1 + i) * sizeof(int64_t), sizeof(int64_t));
  }

  return true;
}
