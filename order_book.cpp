#include "order_book.h"
#include <cstdlib>
#include <iomanip>

const MboRow parse_line_to_mbo(const std::string& line) {
    std::stringstream ss(line);
    MboRow row;
    
    std::string* fields[] = {
        &row.ts_recv, &row.ts_event, &row.rtype, &row.publisher_id,
        &row.instrument_id, &row.action, &row.side, &row.price,
        &row.size, &row.channel_id, &row.order_id, &row.flags,
        &row.ts_in_delta, &row.sequence, &row.symbol
    };
    
    for (auto* field : fields) {
        std::getline(ss, *field, ',');
    }
    return row;
}

void process_header_line(std::ifstream& inFile, std::ofstream& outFile) {
    std::string line;
    std::getline(inFile, line); // skip the mbo header

    // header line
    std::vector<std::string> baseColumns = {
        "ts_recv", "ts_event", "rtype", "publisher_id",
        "instrument_id", "action", "side", "depth", "price", "size", 
        "flags", "ts_in_delta", "sequence"
    };
    
    for (const auto& col : baseColumns) {
        outFile << "," << col;
    }
    
    for (int i = 0; i < 10; ++i) {
        std::string suffix = "0" + std::to_string(i);
        outFile << ",bid_px_" << suffix << ",bid_sz_" << suffix << ",bid_ct_" << suffix;
        outFile << ",ask_px_" << suffix << ",ask_sz_" << suffix << ",ask_ct_" << suffix;
    }
    
    outFile << ",symbol,order_id" << std::endl;
}

void OrderBook::add(double price, int size, char side) {
    if (side == 'B') {
        auto& bucket = bids[price]; 
        bucket.first += size;
        bucket.second++;
    } else if (side == 'A') {
        auto& bucket = asks[price]; 
        bucket.first += size;
        bucket.second++;
    }
}

void OrderBook::cancel(double price, int size, char side) {
    if (side == 'B') {
        auto& bucket = bids[price];
        bucket.first -= size;
        bucket.second--;
        if (bucket.first <= 0) bids.erase(price);
    }
    if (side == 'A') {
        auto& bucket = asks[price];
        bucket.first -= size;
        bucket.second--;
        if (bucket.first <= 0) asks.erase(price);
    }
}

int OrderBook::calculate_depth(std::string price, char side) const {
    if (price.empty()){
        return 0;
    }
    double price_double = std::stod(price);
    if (side == 'B') {
        int depth = 0;
        for (const auto& bucket : bids) {
            if (depth > 10) break;
            if (price_double >= bucket.first) {
                return depth;
            }
            depth++;
        }
        return depth;
    } else { 
        int depth = 0;
        for (const auto& bucket : asks) {
            if (depth > 10) break;
            if (price_double <= bucket.first) { 
                return depth;
            }
            depth++;
        }
        return depth;
    }
}


// MBPFormatter class implementations
std::string MBPFormatter::remove_trailing_zeros(const std::string& s) {
    if (s.empty()) {
        return "";
    }
    size_t dot_pos = s.find('.');
    if (dot_pos == std::string::npos) {
        return s + ".0"; 
    }
    size_t last_not_zero = s.find_last_not_of('0');
    if (last_not_zero == dot_pos) {
        return s.substr(0, dot_pos + 2);
    }
    return s.substr(0, last_not_zero + 1);
}

std::vector<std::string> MBPFormatter::generate_top_10_snapshot(const OrderBook& book) {
    std::vector<std::string> row(60);
    auto bid_iterator = book.bids.begin();
    auto ask_iterator = book.asks.begin();
    
    for (int i = 0; i < 10; ++i) {
        int startIdx = i * 6;
        if (i < book.bids.size()){
            row[startIdx] = remove_trailing_zeros(std::to_string(bid_iterator->first));
            row[startIdx + 1] = std::to_string(bid_iterator->second.first);
            row[startIdx + 2] = std::to_string(bid_iterator->second.second);
            bid_iterator++;
        } else {
            row[startIdx] = "";
            row[startIdx + 1] = "0";
            row[startIdx + 2] = "0";
        }
        if (i < book.asks.size()){
            row[startIdx + 3] = remove_trailing_zeros(std::to_string(ask_iterator->first));
            row[startIdx + 4] = std::to_string(ask_iterator->second.first);
            row[startIdx + 5] = std::to_string(ask_iterator->second.second);
            ask_iterator++;
        } else {
            row[startIdx + 3] = "";
            row[startIdx + 4] = "0";
            row[startIdx + 5] = "0";
        }
    }
    return row;
}
  

void MBPFormatter::generate_mbp_row(std::ofstream& outFile, const MboRow& mboRow, 
                                   const OrderBook& book, int rowIndex, bool is_trade) {
    std::vector<std::string> row(76);
    std::vector<std::string> current_snapshot = generate_top_10_snapshot(book);

    row[0] = std::to_string(rowIndex);
    row[1] = mboRow.ts_event;
    row[2] = mboRow.ts_event;
    row[3] = "10";
    row[4] = mboRow.publisher_id;
    row[5] = mboRow.instrument_id;
    row[6] = (is_trade) ? "T" : mboRow.action;
    row[7] = mboRow.side;
    row[8] = std::to_string(book.calculate_depth(mboRow.price, mboRow.side[0]));
    row[9] = remove_trailing_zeros(mboRow.price);
    row[10] = mboRow.size;
    row[11] = mboRow.flags;
    row[12] = mboRow.ts_in_delta;
    row[13] = mboRow.sequence;
    
    int bookIndex = 14;
    for (auto& value : current_snapshot){
        row[bookIndex] = value;
        bookIndex++;
    }

    row[74] = mboRow.symbol;
    row[75] = mboRow.order_id;

    for (size_t i = 0; i < row.size(); ++i) {
        outFile << row[i] << (i == row.size() - 1 ? "\n" : ",");
    }
}

bool MBPFormatter::check_snapshot_changed(std::vector<std::string>& previous_snapshot, 
                                         const OrderBook& book) {
    std::vector<std::string> current_snapshot = generate_top_10_snapshot(book);
    if (previous_snapshot == current_snapshot) {
        return false;
    } else {
        previous_snapshot = current_snapshot;
        return true;
    }
}

bool MBPFormatter::handle_tfc_cases(const MboRow& currentRow, OrderBook& orderBook, std::ofstream& outFile, MBPFormatter& formatter, int& cached_tfc_rows, int& rowIndex){
    if (currentRow.action == ACTION_TRADE && currentRow.side == SIDE_NONE) {
        return true;
    }
    
    if ((currentRow.action == ACTION_TRADE && currentRow.side != SIDE_NONE) || 
        (currentRow.action == ACTION_FILL)) {
        cached_tfc_rows++;
        return true;
    }
    
    if (cached_tfc_rows == 2 && currentRow.action == ACTION_CANCEL) {
        orderBook.cancel(std::stod(currentRow.price), std::stoi(currentRow.size), currentRow.side[0]);
        cached_tfc_rows = 0;
        formatter.generate_mbp_row(outFile, currentRow, orderBook, rowIndex, true);
        return true;
    }
    return false;
}

// a helper function to help me debug and check the order book
void MBPFormatter::print_book(const OrderBook& book) const {
    // print asks
    std::vector<std::pair<double, std::pair<int, int>>> temp_asks;
    for (const auto& ask_level : book.asks) {
        temp_asks.push_back({ask_level.first, ask_level.second});
    }

    std::cout << "ASKS (" << book.asks.size() << " levels):" << std::endl;
    if (temp_asks.empty()) {
        std::cout << "  (empty)" << std::endl;
    } else {
        int count = 0;
        for (auto it = temp_asks.rbegin(); it != temp_asks.rend(); ++it) {
            if (count == 10) {
                std::cout << "  ******** End of Top 10 ********" << std::endl;
            }
            std::cout << "  [" << count << "]\t"
                      << "Price: " << it->first
                      << "\t| Size: " << it->second.first
                      << "\t| Count: " << it->second.second << std::endl;
            count++;
        }
    }

    // print bids
    std::cout << "------------------------------------" << std::endl;
    std::cout << "BIDS (" << book.bids.size() << " levels):" << std::endl;
    if (book.bids.empty()) {
        std::cout << "  (empty)" << std::endl;
    } else {
        int count = 0;
        for (const auto& bid_level : book.bids) {
            if (count == 10) {
                std::cout << "  ******** End of Top 10 ********" << std::endl;
            }
            std::cout << "  [" << count << "]\t"
                      << "Price: " << bid_level.first
                      << "\t| Size: " << bid_level.second.first
                      << "\t| Count: " << bid_level.second.second << std::endl;
            count++;
        }
    }
    std::cout << "====================================" << std::endl;
} 