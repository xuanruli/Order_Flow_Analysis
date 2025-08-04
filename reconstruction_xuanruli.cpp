#include "order_book.h"

int main(int argc, char* argv[]) {
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    if (argc < 2) {
        std::cerr << "not enough arguments"<< std::endl;
        return 1;
    }

    std::ofstream outFile("mbp_reconstruction.csv");
    std::ifstream inFile(argv[1]);
    
    if (!inFile.is_open() || !outFile.is_open()) {
        std::cerr << "Error opening files!" << std::endl;
        return 1;
    }

    OrderBook orderBook;
    MBPFormatter formatter;
    process_header_line(inFile, outFile);
    int cached_tfc_rows = 0;
    int rowIndex = 0;
    std::string line;
    while (std::getline(inFile, line)) {
        MboRow currentRow = parse_line_to_mbo(line);
        
        // handle T-F-C cases
        if (formatter.handle_tfc_cases(currentRow, orderBook, outFile, formatter, cached_tfc_rows, rowIndex)) {
            continue;
        }
        
        // handle add and cancel cases
        if (currentRow.action == ACTION_ADD) {
            orderBook.add(std::stod(currentRow.price), std::stoi(currentRow.size), currentRow.side[0]);
        }
        if (currentRow.action == ACTION_CANCEL){
            orderBook.cancel(std::stod(currentRow.price), std::stoi(currentRow.size), currentRow.side[0]);
        }

        // only generate mbp row if the depth is less than 10
        if (orderBook.calculate_depth(currentRow.price, currentRow.side[0]) < MAX_BOOK_DEPTH){
            formatter.generate_mbp_row(outFile, currentRow, orderBook, rowIndex, false);
            rowIndex++;
        }  
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Execution time: " << duration.count() << " ms" << std::endl;
    
    return 0;
} 