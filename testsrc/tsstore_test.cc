#include <iostream>

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

  void dump(int64_t offset, int64_t length) {
    std::cout << "[" << length << "@" << offset << "]: ";
    for (int64_t i = 0; i < length; ++i) {
      std::cout << (char)data_[offset + i] << " ";
    }
    std::cout << std::endl;
  }

  virtual int64_t Write(int64_t offset, int64_t length, const void* buf) override {
    std::cout << "W " << length << "@" << offset << std::endl;
    memcpy(data_ + offset, buf, length);
    dump(offset, length);
    return length;
  }

  virtual int64_t Read(int64_t offset, int64_t length, void* buf) override { 
    std::cout << "R " << length << "@" << offset << std::endl;
    dump(offset, length);
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

TEST(TSStoreTest, WriteAndReadBack) {
  RamBlockDevice* bd = new RamBlockDevice(10LL << 20);
  TSStore store(TSStore::Options(), bd);

  SeriesSpec spec;
  spec.name = "fooseries";

  Column column;
  column.name = "data";
  column.type = INT64;
  spec.columns.push_back(column);

  TSID id = store.CreateSeries(spec);
  if (id) {  }
  std::unique_ptr<TSWriter> writer(store.OpenWriter("fooseries"));

  std::vector<int64_t> d1 { 999999999 };
  EXPECT_TRUE(writer->Write(1234567890, d1));

  std::vector<int64_t> d2 { 88888888 };
  EXPECT_TRUE(writer->Write(1234567891, d2));

  std::unique_ptr<TSReader> reader = store.OpenReader("fooseries");
  int64_t timestamp_out;
  std::vector<int64_t> data_out;

  EXPECT_TRUE(reader->Next(&timestamp_out, &data_out));
  EXPECT_EQ(1234567890, timestamp_out);
  EXPECT_EQ(1, data_out.size());
  EXPECT_EQ(999999999, data_out[0]);

  EXPECT_TRUE(reader->Next(&timestamp_out, &data_out));
  EXPECT_EQ(1234567891, timestamp_out);
  EXPECT_EQ(1, data_out.size());
  EXPECT_EQ(88888888, data_out[0]);

  EXPECT_FALSE(reader->Next(&timestamp_out, &data_out));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
