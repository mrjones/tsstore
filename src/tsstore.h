#include <map>
#include <memory>
#include <string>
#include <vector>

class BlockDevice {
 public:
  BlockDevice() { }
  virtual ~BlockDevice() { }

  virtual int64_t Write(int64_t offset, int64_t length, const void* data) = 0;
  virtual int64_t Read(int64_t offset, int64_t length, void* out) = 0;
};

class TSStore;

typedef int64_t TSID;

class TSWriter {
 public:
  TSWriter(TSID id);

  // TODO: make this better
  // - varargs or templates?
  // - variable sized data?
  bool Write(int64_t timestamp, std::vector<int64_t> data);

 private:
  TSID series_id_;
};

class TSReader {
 public:
  TSReader(TSID id);

  bool Next(int64_t* timestamp_out, std::vector<int64_t>* data_out);

 private:
  TSID series_id_;
};

class TSStore {
 public:
  struct Options {
    Options() : block_size(1L << 20) { }

    int64_t block_size;
  };

  TSStore(const Options& options, BlockDevice* device);
  ~TSStore();

  std::unique_ptr<TSWriter> OpenWriter(const std::string& name) {
    auto i = series_ids_.find(name);
    if (i == series_ids_.end()) {
      return nullptr;
    }

    return std::unique_ptr<TSWriter>(new TSWriter(i->second));
  }

  std::unique_ptr<TSReader> OpenReader(const std::string& name) {
    return nullptr;
  }

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

  TSID CreateSeries(const SeriesSpec& spec) {
    TSID id = next_id_++;

    Timeseries* timeseries = &(series_[id]);
    timeseries->spec = spec;
    
    return id;
  }

 private:
  struct Block {
    int64_t offset;
    int64_t length;
  };

  typedef int64_t Timestamp;

  struct Cursor {
    Block* block;
    int64_t block_offset;
  };

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
