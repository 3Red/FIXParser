#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <tuple>

using namespace std;
using namespace chrono;

constexpr int  TAG_MSG_TYPE = 35;
constexpr int  TAG_ORDER_QTY = 38;

constexpr char MSG_TYPE_EXECUTION_REPORT[] = "8";

constexpr char MSG_END[] = "\00110=";
constexpr int  CHECKSUM_LENGTH = 3; // number of characters in the value of a checksum

constexpr char SOH = '\001';              // pattern that delimits tag/value pairs
constexpr char TAG_VALUE_SEPARATOR = '='; // pattern that separates tag from value


struct MessageParser
{
    MessageParser(string& data, size_t begin, size_t end)
        : data_(data)
        , begin_(begin)
        , end_(end)
    {
        parse_fields();
    }

    MessageParser& operator=(const MessageParser& rhs)
    {
        fields_ = rhs.fields_;
        data_  = rhs.data_;
        begin_ = rhs.begin_;
        end_   = rhs.end_;

        return *this;
    }

    bool is_valid() { return begin_ != end_; }

    const string& find(int tag)
    {
        for(auto &i : fields_)
        {
            if (get<0>(i) == tag) { return get<1>(i); }
        }

        static string empty;
        return empty;
    }

private:
    void parse_fields()
    {
        size_t tag_begin = begin_;
        while (tag_begin < end_)
        {
            // The tag is the string from the current position until the equal sign
            size_t tag_end = data_.find(TAG_VALUE_SEPARATOR, tag_begin);
            string tag = data_.substr(tag_begin, tag_end - tag_begin);

            // The value is the string from just after the equal sign to the SOH
            size_t value_begin = tag_end + 1;
            size_t value_end = data_.find(SOH, value_begin);
            string field = data_.substr(value_begin, value_end - value_begin);

            // Store the tag and value in our container
            fields_.emplace_back(stoi(tag), field);

            // Prepare to look for the next tag
            tag_begin = value_end + 1;
        }
    }

    vector<tuple<int, string>> fields_;
    string& data_;
    size_t  begin_;
    size_t  end_;
};


struct FileParser
{
    FileParser(string& data)
        : data_(data)
        , index_(0)
    {}

    MessageParser next()
    {
        size_t start_of_message = index_;

        // Look for the end of message pattern
        size_t end_of_message = data_.find(MSG_END, index_);
        if (end_of_message == string::npos)
        {
            return MessageParser(data_, 0, 0);
        }

        // The complete message includes the end of message pattern,
        // the checksum value and the terminating SOH
        end_of_message += sizeof(MSG_END) + CHECKSUM_LENGTH;
        index_ = end_of_message;

        return MessageParser(data_, start_of_message, end_of_message);
    }

private:
    string& data_;
    size_t  index_;
};


struct Accumulator
{
    Accumulator()
        : quantity_(0)
    {}

    void add(MessageParser& parser)
    {
        // If the parsed message is an Execution Report
        if (parser.find(TAG_MSG_TYPE) == MSG_TYPE_EXECUTION_REPORT)
        {
            // Add the Order Qty value to our sum
            quantity_ += stoi(parser.find(TAG_ORDER_QTY));
        }
    }

    int get() { return quantity_; }

private:
    int quantity_;
};


int main(int argc, char *argv[])
{
    // Read example data from file
    ifstream t("data/FIX.4.2-ICE-12000.body");
    string data((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());

    Accumulator accumulator;

    FileParser file_parser(data);

    // Parse each individual message found in the file
    auto begin = chrono::high_resolution_clock::now();
    vector<nanoseconds> times;
    MessageParser message = file_parser.next();
    while (message.is_valid())
    {
        // Parse each individual message found in the file
        accumulator.add(message);

        // Get and parse the next message
        auto message_begin(high_resolution_clock::now());
        message = file_parser.next();
        auto message_end = high_resolution_clock::now();

        auto duration = duration_cast<nanoseconds>(message_end - message_begin).count();
        times.emplace_back(duration);
    }

    // Write summary statistics
    auto end = high_resolution_clock::now();
    auto total_duration = duration_cast<nanoseconds>(end - begin).count();
    cout << "total qty:     " << accumulator.get() << endl;
    cout << "duration(ns):  " << total_duration << endl;
    cout << "ns/msg:        " << double(total_duration)/times.size() << endl;

    // Write the timing data to a file
    ofstream times_file("times-3.txt");
    for(auto i : times)
    {
        times_file << i.count() << endl;
    }

    return 0;
}
