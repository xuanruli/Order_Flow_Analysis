Run
---------------------
- make && ./reconstruction_xuanruli mbo.csv
- make test


In my reconstruction file, I tried with several data structure and methods to increase the speed.

Data Structure Choice
---------------------
For the data structure of order book, I choose map because in C++ the map's insert and delete is O(log n) because
it's a red black tree in lower level, which could benefits our case such that add order can be intensively.
Although the find is also O(logN) but we can benefit from following condition check (when the order book is large, 
we can utilize the order map to easily find top - 10 level), I stick on map.

Performance Bottleneck & Optimization
-------------------------------------
### Phase 1: Snapshot Comparison

Initially, I used a full top-10 snapshot comparison strategy: at each update, a new snapshot of the top 10 levels (price, size, and order count for both bid and ask sides) is generated and compared with the previous one. If any difference is detected, a new MBP (market by price) row is written.

- Complexity: `O(60)` per update (10 levels * 2 sides * 3 fields)
- Execution Time: **140 ms** average on my local machine

### Phase 2: Depth-based Filtering (Optimized)

To reduce redundant MBP writes, I leveraged the fact that if an order lies outside the top 10 depth, we don’t need to record it in the MBP stream. 

By traversing the sorted `std::map` up to 10 levels and returning early if the price does not fall within the top 10, I effectively turned depth-checking into a nearly constant-time operation (early exit in most cases).

- Worst-case Complexity: `O(10)`
- Average-case: **~O(1)** due to early return
- Execution Time: **94 ms** average

-------------------------------------
* Except this, I also think about some minor optimization step like ifstream instead of fstream, use an int to record T-F-C case, 
and use string [] to parse every line and so on


