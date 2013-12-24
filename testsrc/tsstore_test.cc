#include "gtest/gtest.h"
#include "../src/tsstore.h"

class RamBlockDevice : public BlockDevice {
public:
  RamBlockDevice(int64_t size_bytes)
  : BlockDevice(),
    size_bytes_(size_bytes),
    data_((char*)malloc(size_bytes)) { }

  virtual ~RamBlockDevice() {
    delete data_;
 }

  virtual int64_t Write(int64_t offset, int64_t length, const void* buf) override {
    memcpy(data_ + offset, buf, length);
    return length;
  }

  virtual int64_t Read(int64_t offset, int64_t length, void* buf) override { 
    memcpy(buf, data_ + offset, length);
    return length;
  }

private:
  int64_t size_bytes_;
  char* data_;
};

TEST(RamBlockDeviceTest, ReadWrite) {
  RamBlockDevice device(10LL << 20);

  const std::string kData = "hello, world!";

  device.Write(0, kData.length(), kData.c_str());
  
  char buf[kData.length()];
  device.Read(0, kData.length(), buf);
  std::string out(buf);

  ASSERT_EQ(kData, out);
}

TEST(TSStoreTest, Foo) {
  TSStore store(new RamBlockDevice(10LL << 20));
  std::unique_ptr<TSWriter> writer = store.OpenWriter("fooseries");

  std::unique_ptr<TSReader> reader = store.OpenReader("fooseries");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
