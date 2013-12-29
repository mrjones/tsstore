#include <map>
#include <memory>
#include <string>
#include <vector>

class TSStore;

class BlockDevice {
 public:
  BlockDevice() { }
  virtual ~BlockDevice() { }

  virtual int64_t Write(int64_t offset, int64_t length, const void* data) = 0;
  virtual int64_t Read(int64_t offset, int64_t length, void* out) = 0;
};

struct Block {
  int64_t offset;
  int64_t length;
};

struct Cursor {
  Block block;
  int64_t block_offset;
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

typedef int64_t TSID;

class TSWriter {
 public:
  TSWriter(TSID id, const SeriesSpec& spec, const Cursor& cursor, BlockDevice* block_device);

  // TODO: make this better
  // - varargs or templates?
  // - variable sized data?
  bool Write(int64_t timestamp, std::vector<int64_t> data);

 private:
  TSID series_id_;
  SeriesSpec spec_;
  Cursor cursor_;
  BlockDevice* block_device_;
};

class TSReader {
 public:
  TSReader(TSID id, const SeriesSpec& spec, const Block& data, BlockDevice* block_device);

  bool Next(int64_t* timestamp_out, std::vector<int64_t>* data_out);

 private:
  TSID series_id_;
  SeriesSpec spec_;
  Cursor cursor_;
  BlockDevice* block_device_;
};

class TSStore {
 public:
  struct Options {
    Options() : block_size(1L << 20) { }

    int64_t block_size;
  };

  TSStore(const Options& options, BlockDevice* device);
  ~TSStore();

  std::unique_ptr<TSWriter> OpenWriter(const std::string& name);
  std::unique_ptr<TSReader> OpenReader(const std::string& name);

  TSID CreateSeries(const SeriesSpec& spec) {
    TSID id = next_id_++;

    Timeseries* timeseries = &(series_[id]);
    timeseries->spec = spec;
    
    series_ids_[spec.name] = id;

    return id;
  }

 private:
  typedef int64_t Timestamp;

  struct Timeseries {
    SeriesSpec spec;
    std::map<Timestamp, Block> blocks;
  };

  Block AllocateBlock();

  const Options options_;
  std::unique_ptr<BlockDevice> device_;
  TSID next_id_;
  std::map<TSID, Timeseries> series_;
  std::map<std::string, TSID> series_ids_;
};
