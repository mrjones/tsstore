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

  virtual int64_t Size() override {
    return size_bytes_;
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
  // TODO: put this logic in SeriesSpec?
  const int kRecordSize = 2 * sizeof(int64_t);  // timestamp + 1 column

  RamBlockDevice* bd = new RamBlockDevice(10LL << 20);
  TSStore::Options options;
  options.segment_size = 2 * kRecordSize;
  TSStore store(options, bd);

  SeriesSpec spec;
  spec.name = "fooseries";

  Column column;
  column.name = "data";
  column.type = INT64;
  spec.columns.push_back(column);

  TSID id = store.CreateSeries(spec);
  if (id) {  }
  std::unique_ptr<TSWriter> writer(store.OpenWriter("fooseries"));

  // Insert more than two records to make sure we span segments
  const int kRecords = 10;
  for (int i = 0; i < kRecords; ++i) {
    std::vector<int64_t> d { 9876543210 + i };
    EXPECT_TRUE(writer->Write(1234567890 + i, d));
  }


  std::unique_ptr<TSReader> reader = store.OpenReader("fooseries");

  int64_t timestamp_out;
  std::vector<int64_t> data_out;
  for (int i = 0; i < kRecords; ++i) {
    timestamp_out = -1;
    data_out.clear();

    EXPECT_TRUE(reader->Next(&timestamp_out, &data_out));
    EXPECT_EQ(1234567890 + i, timestamp_out);
    EXPECT_EQ(1, data_out.size());
    if (data_out.size() > 0) {
      EXPECT_EQ(9876543210 + i, data_out[0]);
    }
  }

  timestamp_out = -1;
  data_out.clear();
  EXPECT_FALSE(reader->Next(&timestamp_out, &data_out));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
