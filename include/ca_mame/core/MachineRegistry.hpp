#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "ca_mame/core/IMachine.hpp"
namespace ca_mame {
class MachineRegistry {
public:
    using Factory = std::unique_ptr<IMachine>(*)();
    struct Entry { std::string id; std::string description; Factory factory; };
    bool registerMachine(std::string id,std::string description,Factory factory);
    std::unique_ptr<IMachine> create(std::string_view id) const;
    const std::vector<Entry>& entries() const noexcept { return entries_; }
private:
    std::vector<Entry> entries_;
};
MachineRegistry makeDefaultRegistry();
}
