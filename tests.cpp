#include "order_book.h"
#include <iostream>
#include <cassert>

void test_calculate_depth_basic() {
    OrderBook book;
    book.add(12.5, 100, 'B');
    book.add(12.0, 200, 'B');
    book.add(11.5, 150, 'B');
    book.add(13.0, 100, 'A');
    book.add(13.5, 200, 'A');
    book.add(14.0, 150, 'A');
    
    assert(book.calculate_depth("12.6", 'B') == 0);
    assert(book.calculate_depth("12.5", 'B') == 0);
    assert(book.calculate_depth("12.2", 'B') == 1);
    assert(book.calculate_depth("12.0", 'B') == 1);
    assert(book.calculate_depth("11.8", 'B') == 2);
    assert(book.calculate_depth("11.5", 'B') == 2);
    assert(book.calculate_depth("11.0", 'B') == 3);
    
    assert(book.calculate_depth("12.9", 'A') == 0);
    assert(book.calculate_depth("13.0", 'A') == 0);
    assert(book.calculate_depth("13.2", 'A') == 1);
    assert(book.calculate_depth("13.5", 'A') == 1);
    assert(book.calculate_depth("13.8", 'A') == 2);
    assert(book.calculate_depth("14.0", 'A') == 2);
    assert(book.calculate_depth("14.5", 'A') == 3);
}

void test_calculate_depth_performance() {
    OrderBook book;
    for (int i = 0; i < 1000; i++) {
        book.add(12.0 - i * 0.001, 100, 'B');
        book.add(13.0 + i * 0.001, 100, 'A');
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
        book.calculate_depth("11.5", 'B');
        book.calculate_depth("13.5", 'A');
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Performance: " << duration.count() / 2000.0 << " μs/query" << std::endl;
}

void test_orderbook_operations() {
    OrderBook book;
    
    book.add(12.5, 100, 'B');
    book.add(12.5, 50, 'B');
    book.add(12.0, 200, 'B');
    book.add(13.0, 100, 'A');
    book.add(13.0, 75, 'A');
    book.add(13.5, 150, 'A');
    
    assert(book.bids.size() == 2);
    assert(book.asks.size() == 2);
    
    auto bid_it = book.bids.find(12.5);
    assert(bid_it != book.bids.end());
    assert(bid_it->second.first == 150);
    assert(bid_it->second.second == 2);
    
    auto ask_it = book.asks.find(13.0);
    assert(ask_it != book.asks.end());
    assert(ask_it->second.first == 175);
    assert(ask_it->second.second == 2);
    
    book.cancel(12.5, 30, 'B');
    bid_it = book.bids.find(12.5);
    assert(bid_it->second.first == 120);
    assert(bid_it->second.second == 1);
    
    book.cancel(12.5, 120, 'B');
    assert(book.bids.find(12.5) == book.bids.end());
    assert(book.bids.size() == 1);
    
    book.cancel(13.0, 200, 'A');
    assert(book.asks.find(13.0) == book.asks.end());
    assert(book.asks.size() == 1);
}

void test_snapshot_generation() {
    OrderBook book;
    MBPFormatter formatter;
    
    auto empty_snapshot = formatter.generate_top_10_snapshot(book);
    assert(empty_snapshot.size() == 60);
    
    for (int i = 0; i < 10; i++) {
        assert(empty_snapshot[i * 6] == "");
        assert(empty_snapshot[i * 6 + 1] == "0");
        assert(empty_snapshot[i * 6 + 2] == "0");
        assert(empty_snapshot[i * 6 + 3] == "");
        assert(empty_snapshot[i * 6 + 4] == "0");
        assert(empty_snapshot[i * 6 + 5] == "0");
    }
    
    book.add(12.5, 100, 'B');
    book.add(12.0, 200, 'B');
    book.add(13.0, 150, 'A');
    book.add(13.5, 300, 'A');
    
    auto snapshot = formatter.generate_top_10_snapshot(book);
    
    assert(snapshot[0] == "12.5");
    assert(snapshot[1] == "100");
    assert(snapshot[2] == "1");
    assert(snapshot[3] == "13.0");
    assert(snapshot[4] == "150");
    assert(snapshot[5] == "1");
    
    assert(snapshot[6] == "12.0");
    assert(snapshot[7] == "200");
    assert(snapshot[8] == "1");
    assert(snapshot[9] == "13.5");
    assert(snapshot[10] == "300");
    assert(snapshot[11] == "1");
}

void test_price_formatting() {
    OrderBook book;
    MBPFormatter formatter;
    
    book.add(14.000, 100, 'B');
    book.add(12.500, 200, 'B');
    book.add(13.000, 150, 'A');
    book.add(15.123, 300, 'A');
    
    auto snapshot = formatter.generate_top_10_snapshot(book);
    
    assert(snapshot[0] == "14.0");
    assert(snapshot[6] == "12.5");
    assert(snapshot[3] == "13.0");
    assert(snapshot[9] == "15.123");
}

void test_mbo_parsing() {
    std::string test_line = "1609459200000000000,1609459200000000000,10,1,123,A,B,12.50,100,1,order123,128,0,1001,AAPL";
    MboRow row = parse_line_to_mbo(test_line);
    
    assert(row.ts_recv == "1609459200000000000");
    assert(row.ts_event == "1609459200000000000");
    assert(row.rtype == "10");
    assert(row.publisher_id == "1");
    assert(row.instrument_id == "123");
    assert(row.action == "A");
    assert(row.side == "B");
    assert(row.price == "12.50");
    assert(row.size == "100");
    assert(row.channel_id == "1");
    assert(row.order_id == "order123");
    assert(row.flags == "128");
    assert(row.ts_in_delta == "0");
    assert(row.sequence == "1001");
    assert(row.symbol == "AAPL");
}

void test_edge_cases() {
    OrderBook book;
    MBPFormatter formatter;
    
    assert(book.calculate_depth("", 'B') == 0);
    assert(book.calculate_depth("", 'A') == 0);
    assert(book.calculate_depth("12.5", 'B') == 0);
    assert(book.calculate_depth("13.0", 'A') == 0);
    
    book.add(12.5, 100, 'X');
    assert(book.bids.empty());
    assert(book.asks.empty());
    
    book.add(12.5, 0, 'B');
    book.add(12.0, -50, 'B');
    book.cancel(99.99, 100, 'B');
    
    book.add(999999.99, 1000000, 'B');
    book.add(0.00001, 1, 'A');
    
    std::vector<std::string> previous_snapshot = formatter.generate_top_10_snapshot(book);
    assert(!formatter.check_snapshot_changed(previous_snapshot, book));
    
    book.add(12.5, 100, 'B');
    assert(formatter.check_snapshot_changed(previous_snapshot, book));
}

void test_deep_book() {
    OrderBook book;
    
    for (int i = 0; i < 15; i++) {
        double price = 12.0 - i * 0.1;
        book.add(price, 100, 'B');
    }
    
    for (int i = 0; i < 15; i++) {
        double price = 13.0 + i * 0.1;
        book.add(price, 100, 'A');
    }
    
    assert(book.calculate_depth("12.1", 'B') == 0);
    assert(book.calculate_depth("11.6", 'B') == 4);
    assert(book.calculate_depth("11.1", 'B') == 9);
    
    assert(book.calculate_depth("12.9", 'A') == 0);
    assert(book.calculate_depth("13.4", 'A') == 4);
    assert(book.calculate_depth("13.9", 'A') == 9);
    
    int deep_bid_depth = book.calculate_depth("10.5", 'B');
    int deep_ask_depth = book.calculate_depth("14.5", 'A');
    
    std::cout << "Deep book test: bid_depth=" << deep_bid_depth << ", ask_depth=" << deep_ask_depth << std::endl;
}

int main() {
    
    try {
        test_calculate_depth_basic();
        std::cout << "depth_basic" << std::endl;
        
        test_calculate_depth_performance();
        std::cout << "depth_performance" << std::endl;
        
        test_orderbook_operations();
        std::cout << "orderbook_ops" << std::endl;
        
        test_snapshot_generation();
        std::cout << "snapshot_gen" << std::endl;
        
        test_price_formatting();
        std::cout << "price_format" << std::endl;
        
        test_mbo_parsing();
        std::cout << "mbo_parsing" << std::endl;
        
        test_edge_cases();
        std::cout << "edge_cases" << std::endl;
        
        test_deep_book();
        std::cout << "deep_book" << std::endl;
        
        std::cout << "All tests passed" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        return 1;
    } catch (...) {
        return 1;
    }
} 