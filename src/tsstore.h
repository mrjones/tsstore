#include <memory>
#include <string>

class BlockDevice {
 public:
  BlockDevice() { }
  virtual ~BlockDevice() { }

  virtual int64_t Write(int64_t offset, int64_t length, const void* data) = 0;
  virtual int64_t Read(int64_t offset, int64_t length, void* out) = 0;
};

class TSWriter {

};

class TSReader {

};

class TSStore {
 public:
  TSStore(BlockDevice* device);
  ~TSStore();

  std::unique_ptr<TSWriter> OpenWriter(const std::string& name) {
    return nullptr;
  }

  std::unique_ptr<TSReader> OpenReader(const std::string& name) {
    return nullptr;
  }

 private:
  std::unique_ptr<BlockDevice> device_;
};
