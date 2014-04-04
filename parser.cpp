#include <string.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <tuple>

using namespace std;
using namespace chrono;

constexpr char EXECUTION_REPORT[] = "8";
constexpr int CHECKSUM_LENGTH = 3;
constexpr char SOH = '\001';
constexpr int MSG_TYPE = 35;
constexpr char MSG_TYPE_CHAR[] = "35";
constexpr int ORDER_QTY = 38;
constexpr char END[] = "\00110=";
constexpr char ORDER_QTY_CHAR[] = "38";

struct MessageParser {
    MessageParser(string& data, size_t begin, size_t end)
        : data_(data)
        , begin_(begin)
        , end_(end) {
        parse_fields();
    }

    MessageParser& operator=(const MessageParser& rhs) {
        return *new(this) MessageParser(rhs.data_, rhs.begin_, rhs.end_);
    }

    bool is_valid() { return begin_ != end_; }

    const string& find(int tag) {
        if (tag == MSG_TYPE) { return message_type_; }
        else if (tag == ORDER_QTY) { return order_qty_; }
        static string empty;
        return empty;
    }

private:
    void parse_fields() {
        auto tag_begin(begin_);
        int found_value_count(0);

        while (tag_begin < end_ && (found_value_count < 2)) {
            auto tag_end = data_.find('=', tag_begin);
            auto value_begin = tag_end + 1;
            auto value_end = data_.find(SOH, value_begin);

            if (0 == memcmp(MSG_TYPE_CHAR, data_.data() + tag_begin, sizeof(MSG_TYPE_CHAR)-1)) {
                message_type_ = data_.substr(value_begin, value_end - value_begin);
                found_value_count++;
            } else if (0 == memcmp(ORDER_QTY_CHAR, data_.data() + tag_begin, sizeof(ORDER_QTY_CHAR)-1)) {
                order_qty_ = data_.substr(value_begin, value_end - value_begin);
                found_value_count++;
            }
            tag_begin = value_end + 1;
        }
    }
    string  message_type_;
    string  order_qty_;
    string& data_;
    size_t  begin_;
    size_t  end_;
};

struct FileParser {
    FileParser(string& data)
        : data_(data)
        , index_(0)
    {}

    MessageParser next() {
        auto old_index = index_;
        auto message_end = data_.find(END, index_);
        if (message_end == string::npos) {
                return MessageParser(data_, 0, 0);
        }
        index_ = message_end + sizeof(END) + CHECKSUM_LENGTH;
        return MessageParser(data_, old_index, index_);
    }

private:
    string& data_;
    size_t  index_;
};

struct Accumulator {
    Accumulator() : quantity_(0) {}
    void add(MessageParser& parser) {
        if (parser.find(MSG_TYPE) == EXECUTION_REPORT) {
            quantity_ += stoi(parser.find(ORDER_QTY));
        }
    }
    int get() { return quantity_; }

private:
    int  quantity_;
};

int main(int argc, char *argv[]) {
    // Read from file.
    ifstream t("data/FIX.4.2-ICE-12000.body");
    string data((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
    FileParser parser(data);
    MessageParser message(parser.next());
    auto begin(chrono::high_resolution_clock::now());
    
    // Parse individual messages.
    vector<nanoseconds> times;
    Accumulator accumulator;
    for (;message.is_valid(); ) {
        auto message_begin(high_resolution_clock::now());
        accumulator.add(message);
        message = parser.next();
        auto message_end(high_resolution_clock::now());
        auto duration = duration_cast<nanoseconds>(message_end - 
                                                   message_begin).count();
        times.emplace_back(duration);
    }

    // Write summary statistics.
    auto end(high_resolution_clock::now());
    auto total_duration = duration_cast<nanoseconds>(end - begin).count();
    cout << "total qty:     " << accumulator.get() << endl;
    cout << "duration(ns):  " << total_duration << endl;
    cout << "ns/msg:        " << double(total_duration)/times.size() << endl;

    ofstream times_file("times-7.txt");
    for(auto i : times) {
        times_file << i.count() << endl;
    }

    return 0;
}
