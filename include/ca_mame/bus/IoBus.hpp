#pragma once
#include <array>
#include <cstdint>
#include <functional>
namespace ca_mame {
class IoBus {
public:
    using ReadHandler=std::function<std::uint8_t()>;
    using WriteHandler=std::function<void(std::uint8_t)>;
    void onRead(std::uint8_t port,ReadHandler handler) { readers_[port]=std::move(handler); }
    void onWrite(std::uint8_t port,WriteHandler handler) { writers_[port]=std::move(handler); }
    std::uint8_t read(std::uint8_t port) const { return readers_[port]?readers_[port]():0xff; }
    void write(std::uint8_t port,std::uint8_t value) const { if(writers_[port]) writers_[port](value); }
private:
    std::array<ReadHandler,256> readers_{};
    std::array<WriteHandler,256> writers_{};
};
}
