#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>

static constexpr const char* ACTION_TRADE = "T";
static constexpr const char* ACTION_ADD = "A"; 
static constexpr const char* ACTION_CANCEL = "C";
static constexpr const char* ACTION_FILL = "F";
static constexpr const char* SIDE_NONE = "N";
static constexpr int MAX_BOOK_DEPTH = 10;

struct MboRow {
    std::string ts_recv;
    std::string ts_event;
    std::string rtype;
    std::string publisher_id;
    std::string instrument_id;
    std::string action;
    std::string side;
    std::string price;
    std::string size;
    std::string channel_id;
    std::string order_id;
    std::string flags;
    std::string ts_in_delta;
    std::string sequence;
    std::string symbol;
};

class OrderBook {
public:
    std::map<double, std::pair<int, int>, std::greater<double>> bids; 
    std::map<double, std::pair<int, int>> asks;

    void add(double price, int size, char side);
    void cancel(double price, int size, char side);
    int calculate_depth(std::string price, char side) const;
};

class MBPFormatter {
private:
    std::string remove_trailing_zeros(const std::string& s);

public:
    std::vector<std::string> generate_top_10_snapshot(const OrderBook& book);
    void generate_mbp_row(std::ofstream& outFile, const MboRow& mboRow, 
                         const OrderBook& book, int rowIndex, bool is_trade);
    bool check_snapshot_changed(std::vector<std::string>& previous_snapshot, 
                               const OrderBook& book);
    bool handle_tfc_cases(const MboRow& currentRow, OrderBook& orderBook, std::ofstream& outFile, MBPFormatter& formatter, int& cached_tfc_rows, int& rowIndex);
    void print_book(const OrderBook& book) const;
};

// Utility functions
const MboRow parse_line_to_mbo(const std::string& line);
void process_header_line(std::ifstream& inFile, std::ofstream& outFile); 