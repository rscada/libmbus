#pragma once

#include <string>
#include <vector>
#include <ostream>

#include "mbus/mbus.h"

namespace libmbus {

struct SlaveInformation
{
  long long id_{};
  std::string Manufacturer;
  std::string Version;
  std::string ProductName;
  std::string Medium;
  int AccessNumber{};
  std::string Status;
  std::string Signature;
};

struct DataRecord
{
  int id_{0};
  int frame_{0};
  std::string function_;
  int storageNumber_{};
  std::string unit_;
  std::string value_;
  std::string timestamp_;
};

class MbusData
{
public:
    void import_frame_data(mbus_frame_data &reply_data, MbusData *info);

    // operator overloading using friend function
    friend void operator<<(std::ostream &mycout, MbusData &d)
    {
        mycout << "data" << std::endl;
        mycout << "slave id: " << d.slave_information_.id_ << std::endl;
    }

    SlaveInformation slave_information_;
    std::vector<DataRecord> data_records_;
};

} // namespace libmbus
