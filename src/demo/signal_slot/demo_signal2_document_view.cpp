#include <iostream>
#include <string>
#include <boost/signals2.hpp>
#include <boost/bind/bind.hpp> // Updated include to Boost.Bind with placeholders

using namespace boost::placeholders; // Use boost::placeholders namespace for _1, _2, etc.

// Document Class
class Document {
public:
    // Redefine signal_t to accept a const std::string& argument
    typedef boost::signals2::signal<void (const std::string&)> signal_t;

    // Connect a subscriber slot to the signal
    boost::signals2::connection connect(const signal_t::slot_type &subscriber) {
        return sig_.connect(subscriber);
    }

    // Append text to the document and emit the signal with the updated text
    void append(const char* s) {
        text_ += s;
        sig_(text_); // Emit the signal, passing the updated text as an argument
    }

    // Retrieve the current text
    const std::string& getText() const {
        return text_;
    }

private:
    signal_t sig_;         // Signal that notifies subscribers with the updated text
    std::string text_;     // Internal storage for the document's text
};

// TextView Class
class TextView {
public:
    TextView(Document& doc) : document_(doc) {
        // Connect the refresh method to the document's signal using boost::bind
        // The refresh method now accepts a const std::string& argument
        connection_ = document_.connect(boost::bind(&TextView::refresh, this, _1));
    }

    ~TextView() {
        // Disconnect the slot upon destruction to prevent dangling connections
        connection_.disconnect();
    }

    // Refresh the view by displaying the document's updated text
    void refresh(const std::string& updated_text) const {
        std::cout << "TextView: " << updated_text << std::endl;
    }

private:
    Document& document_;                          // Reference to the associated Document
    boost::signals2::connection connection_;      // Connection object for managing the slot
};

// HexView Class
class HexView {
public:
    HexView(Document& doc) : document_(doc) {
        // Connect the refresh method to the document's signal using boost::bind
        // The refresh method now accepts a const std::string& argument
        connection_ = document_.connect(boost::bind(&HexView::refresh, this, _1));
    }

    ~HexView() {
        // Disconnect the slot upon destruction to prevent dangling connections
        connection_.disconnect();
    }

    // Refresh the view by displaying the document's updated text in hexadecimal format
    void refresh(const std::string& updated_text) const {
        std::cout << "HexView:";
        for (unsigned char c : updated_text) {
            std::cout << ' ' << std::hex << static_cast<int>(c);
        }
        std::cout << std::dec << std::endl; // Reset to decimal formatting
    }

private:
    Document& document_;                          // Reference to the associated Document
    boost::signals2::connection connection_;      // Connection object for managing the slot
};

int main(int argc, char* argv[]) {
    Document doc;          // Instantiate the Document
    TextView v1(doc);      // Instantiate the TextView and connect it to the Document
    HexView v2(doc);       // Instantiate the HexView and connect it to the Document

    // Append text to the document. If a command-line argument is provided, use it; otherwise, default to "Hello world!"
    doc.append(argc == 2 ? argv[1] : "Hello world!");

    return 0; // Indicate successful program termination
}
