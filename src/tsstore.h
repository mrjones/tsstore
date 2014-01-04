#include <map>
#include <memory>
#include <string>
#include <vector>

class TSStore;

typedef int64_t TSID;
typedef int64_t Timestamp;

class BlockDevice {
 public:
  BlockDevice() { }
  virtual ~BlockDevice() { }

  virtual int64_t Write(int64_t offset, int64_t length, const void* data) = 0;
  virtual int64_t Read(int64_t offset, int64_t length, void* out) = 0;
  virtual int64_t Size() = 0;
};

struct Segment {
  int64_t offset;
  int64_t length;
};

/* class DataSegmentAllocator { */
/*  public: */
/*   DataSegmentAllocator() { } */
/*   virtual ~DataSegmentAllocator() { } */

/*   virtual bool AllocateDataSegment(TSID tsid, Timestamp first_timestamp, Segment* out) = 0; */
/* }; */

struct Cursor {
  Segment segment;
  int64_t segment_offset;
  bool valid;
};

enum DataType {
  INT64
};

struct Column {
  std::string name;
  DataType type;
};

struct SeriesSpec {
  std::string name;
  std::vector<Column> columns;
};

class SeriesIndex;

class TSWriter {
 public:
  TSWriter(TSID id,
           const SeriesSpec& spec,
           const Cursor& cursor,
           BlockDevice* block_device,
           SeriesIndex* index);

  // TODO: make this better
  // - varargs or templates?
  // - variable sized data?
  bool Write(int64_t timestamp, std::vector<int64_t> data);

 private:
  TSID series_id_;
  SeriesSpec spec_;
  Cursor cursor_;
  BlockDevice* block_device_;
  SeriesIndex* index_;
};

class TSReader {
 public:
  // TODO: locking issues around index.
  TSReader(TSID id, const SeriesSpec& spec, SeriesIndex* index, BlockDevice* block_device);

  bool Next(int64_t* timestamp_out, std::vector<int64_t>* data_out);

 private:
  TSID series_id_;
  SeriesSpec spec_;
  SeriesIndex* index_;  // not owned
  Cursor cursor_;
  BlockDevice* block_device_;
  Timestamp timestamp_;
};

class SegmentLayoutManager {// : public DataSegmentAllocator {
 public:
  SegmentLayoutManager(int64_t device_size, int64_t segment_size);
  virtual ~SegmentLayoutManager();

  bool AllocateDataSegment(TSID tsid, Timestamp first_timestamp, Segment* out);

  bool AllocateSeriesIndexSegment(TSID tsid, Segment* out);

 private:
  struct SegmentInfo {
    enum Kind { EMPTY, SERIES_DATA, SERIES_INDEX };

    Kind kind;
    TSID tsid;

    // Only for data segments
    Timestamp initial_timestamp;
  };

  const int64_t device_size_;
  const int64_t segment_size_;
  std::vector<SegmentInfo> segments_;
};

class SeriesIndex {
 public:
  // does not take ownership of allocator
  SeriesIndex(TSID id, const Segment& location, SegmentLayoutManager* allocator)
    : id_(id), location_(location), allocator_(allocator) { }

  bool AllocateSegment(Timestamp first_timestamp, Segment* out) {
    bool result = allocator_->AllocateDataSegment(id_, first_timestamp, out);
    if (!result) {
      return false;
    }

    // TODO: what if this overrides??
    // TODO: copy ctor?
    Segment* s = &(data_[first_timestamp]);
    s->offset = out->offset;
    s->length = out->length;
    return true;
  }

  bool Next(Timestamp last, Timestamp* ts_out, Segment* segment_out) {
    auto i = data_.upper_bound(last);
    if (i == data_.end()) {
      return false;
    }

    *ts_out = i->first;
    *segment_out = i->second;
    return true;
  }

 private:
  TSID id_;
  Segment location_;
  SegmentLayoutManager* allocator_;
  std::map<Timestamp, Segment> data_;
};

class TSStore {
 public:
  struct Options {
    Options() : segment_size(1L << 20) { }

    int64_t segment_size;
  };

  TSStore(const Options& options, BlockDevice* device);
  ~TSStore();

  std::unique_ptr<TSWriter> OpenWriter(const std::string& name);
  std::unique_ptr<TSReader> OpenReader(const std::string& name);

  TSID CreateSeries(const SeriesSpec& spec);

 private:
  struct Timeseries {
    SeriesSpec spec;
    // make this not nullable, with a construcor or something
    std::unique_ptr<SeriesIndex> index;
    std::map<Timestamp, Segment> segments;
  };

  Segment AllocateSegment();

  const Options options_;
  std::unique_ptr<BlockDevice> device_;
  TSID next_id_;
  std::map<TSID, Timeseries> series_;
  std::map<std::string, TSID> series_ids_;
  SegmentLayoutManager segment_layout_;
};
