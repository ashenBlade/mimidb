#pragma once

#include "IFormatter.hpp"
#include "packets/TupleDescriptionPacket.hpp"
#include <ostream>

namespace mi::client {
class TableFormatter : public IFormatter {
  private:
    std::ostream &_output;

    // Write value for column with padding
    void writeColumnValue(const std::string &value, size_t width, bool align);
  public:
    TableFormatter(std::ostream &output) : _output(output) {};
    void
    Format(const interface::libmimi::TupleDescriptionPacket &descriptor,
           const std::vector<std::unique_ptr<interface::libmimi::DataRowPacket>> &rows) override;

    ~TableFormatter() override = default;
};
} // namespace mi::client
