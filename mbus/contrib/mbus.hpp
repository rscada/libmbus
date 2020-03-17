#pragma once

#include "mbus/contrib/mbusdata.hpp"
#include "mbus/mbus.h"

#include <exception>
#include <sstream>
#include <string_view>

namespace libmbus {

enum class ConnectionType { kUnknown, kSerial, kTCP };

class MBus {
public:
  MBus(const std::string &serial_device);
  MBus(const std::string &host, const uint16_t port);

  ~MBus();

  void setDebug(const bool debug);

  void connect();

  void setBaudrate(long baudrate);

  bool initSlaves();

  mbus_handle *getNativeHandle() const;
  void setNativeHandle(mbus_handle *value);

  bool isSecondaryAddress(const std::string &addr_str);

  int getAddress();
  void setAddress(const std::string &addr_str);

  void sendRequestFrame();

  int recvFrame(mbus_frame &reply);

private:
  mbus_handle *handle_{nullptr};
  bool debug_{false};
  int address_{-1};
  ConnectionType connection_type_{ConnectionType::kUnknown};
};

} // namespace libmbus
