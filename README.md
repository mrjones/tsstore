tsstore
=======

Storage for timeseries data

tsfs
====

Filesystem for storing timeseries data


Offset 0 -> Superblock

Block size
Segment size

Index segment

Data segment


Superblock
- Two superblocks, write one at a time.
- Layout:
- 0000-0001 Number of timeseries
- [ For each timeseries ]
- 8 byte: pointer to index segment
- 
- final 8 bytes: timestamp 

Index Segment
- 

-----
Series metadata:
- Human Name
- ID
- Schema: [Name, Type] --> Entry Size
  Types: int64 / float64, smaller types?
- 
