#include "TableFormatter.hpp"
#include <cassert>

using namespace mi::client;

void TableFormatter::writeColumnValue(const std::string &value, size_t width, bool align) {
    assert(value.size() + 2 <= width);

    auto spacesCount = width - value.size();
    
    auto leftSpaces = align ? spacesCount / 2 : 1;
    auto rightSpaces = spacesCount - leftSpaces;

    this->_output << std::string(leftSpaces, ' ');
    this->_output << value;
    this->_output << std::string(rightSpaces, ' ');
}

void TableFormatter::Format(
    const interface::libmimi::TupleDescriptionPacket &descriptor,
    const std::vector<std::unique_ptr<interface::libmimi::DataRowPacket>> &rows) {

    // For pretty printing we calculate widths of all columns
    auto maxWidths = std::vector<size_t>{};

    // Count in column names first
    for (const auto &attr : descriptor.Attributes()) {
        maxWidths.push_back(attr.Name().size());
    }

    // Then calculate statistics for each tuple
    const auto natts = descriptor.Attributes().size();
    for (const auto &row : rows) {
        const auto &values = row->Values();
        for (auto attno = 0U; attno < natts; ++attno) {
            auto &value = values[attno];
            size_t valWidth;
            if (!value) {
                valWidth = 0;
            } else {
                valWidth = value.value().size();
            }

            if (maxWidths[attno] < valWidth) {
                maxWidths[attno] = valWidth;
            }
        }
    }

    // Add spaces around columns
    for (auto &width : maxWidths) {
        width += 2;
    }

    // Write header
    auto &attrs = descriptor.Attributes();
    for (auto attno = 0U; attno < natts; ++attno) {
        auto &attr = attrs[attno];
        this->writeColumnValue(attr.Name(), maxWidths[attno], true);

        // Add separator between columns
        if (attno < natts - 1) {
            this->_output << '|';
        }
    }

    this->_output << std::endl;

    // Add separator between columns and header
    for (auto attno = 0U; attno < natts; ++attno) {
        auto width = maxWidths[attno];
        this->_output << std::string(width, '-');
        if (attno < natts - 1) {
            this->_output << '+';
        }
    }

    this->_output << std::endl;

    // Now write all tuples
    for (const auto &row : rows) {
        auto &values = row->Values();
        for (auto attno = 0U; attno < natts; ++attno) {
            auto width = maxWidths[attno];
            auto &value = values[attno];
            if (value) {
                this->writeColumnValue(value.value(), width, false);
            } else {
                this->_output << std::string(width, ' ');
            }

            if (attno < natts - 1) {
                this->_output << '|';
            }
        }

        this->_output << std::endl;
    }
}