#include "mbus/contrib/mbus.hpp"

#include <iostream>

namespace libmbus {

MBus::MBus(const std::string &serial_device) {
  handle_ = mbus_context_serial(serial_device.c_str());

  if (handle_ == nullptr) {
    throw std::runtime_error(
        std::string("Could not initialize M-Bus context: ") + mbus_error_str());
  }

  connection_type_ = ConnectionType::kSerial;
}

MBus::MBus(const std::string &host, const uint16_t port) {
  handle_ = mbus_context_tcp(host.c_str(), port);

  if (handle_ == nullptr) {
    throw std::runtime_error(
        std::string("Could not initialize M-Bus context: ") + mbus_error_str());
  }

  connection_type_ = ConnectionType::kTCP;
}

MBus::~MBus() {
  mbus_disconnect(handle_);
  mbus_context_free(handle_);
}

void MBus::setDebug(const bool debug) {
  if (!handle_)
    throw std::runtime_error("native handle not initalized");

  if (debug) {
    mbus_register_send_event(handle_, &mbus_dump_send_event);
    mbus_register_recv_event(handle_, &mbus_dump_recv_event);
  }

  debug_ = debug;
}

void MBus::connect() {
  if (mbus_connect(handle_) == -1) {
    throw std::runtime_error("Failed to setup connection to M-bus gateway");
  }
}

void MBus::setBaudrate(long baudrate) {
  if (mbus_serial_set_baudrate(handle_, baudrate) == -1) {
    throw std::runtime_error("Failed to set baud rate.");
  }
}

bool MBus::initSlaves() {
  auto init_slaves = [this](mbus_handle *handle) {
    if (debug_)
      std::cout << __PRETTY_FUNCTION__ << ": debug: sending init frame #1"
                << std::endl;

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 1) == -1) {
      return false;
    }

    //
    // resend SND_NKE, maybe the first get lost
    //
    if (debug_)
      printf("%s: debug: sending init frame #2\n", __PRETTY_FUNCTION__);

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 1) == -1) {
      return false;
    }

    return true;
  };

  if (init_slaves(handle_) == false) {
    mbus_disconnect(handle_);
    mbus_context_free(handle_);
    return true;
  }
  return false;
}

mbus_handle *MBus::getNativeHandle() const { return handle_; }

void MBus::setNativeHandle(mbus_handle *value) { handle_ = value; }

bool MBus::isSecondaryAddress(const std::string &addr_str) {
  return mbus_is_secondary_address(addr_str.c_str());
}

void MBus::setAddress(const std::string &addr_str) {
  if (isSecondaryAddress(addr_str)) {
    // secondary addressing
    const int ret = mbus_select_secondary_address(handle_, addr_str.data());
    std::stringstream ss;

    if (ret == MBUS_PROBE_COLLISION) {
      ss << __PRETTY_FUNCTION__ << ": Error: The address mask" << addr_str
         << "matches more than one device.";
      throw std::runtime_error(ss.str());

    } else if (ret == MBUS_PROBE_NOTHING) {
      ss << __PRETTY_FUNCTION__
         << ": Error: The selected secondary address does not match any device "
            "["
         << addr_str << "]." << std::endl;
      throw std::runtime_error(ss.str());

    } else if (ret == MBUS_PROBE_ERROR) {
      ss << __PRETTY_FUNCTION__
         << ": Error: Failed to select secondary address [" << addr_str << "]."
         << std::endl;
      throw std::runtime_error(ss.str());
    }
    // else MBUS_PROBE_SINGLE

    address_ = MBUS_ADDRESS_NETWORK_LAYER;
  } else {
    // primary addressing
    address_ = atoi(addr_str.data());
  }
}

int MBus::getAddress() {
  if (address_ == -1) {
    throw std::runtime_error("address not set yet");
  }

  return address_;
}

void MBus::sendRequestFrame() {
  if (mbus_send_request_frame(handle_, address_) == -1) {
    throw std::runtime_error("Failed to send M-Bus request frame.");
  }
}

int MBus::recvFrame(mbus_frame &reply) {
  return mbus_recv_frame(handle_, &reply);
}

} // namespace libmbus
