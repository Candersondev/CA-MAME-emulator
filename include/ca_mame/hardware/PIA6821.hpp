#pragma once
#include <array>
#include <cstdint>
namespace ca_mame::hardware {
// MC6821 register and interrupt model. Control-register bit 2 selects
// data register versus DDR. CA1/CB1 edge events latch IRQ flags.
class PIA6821 {
public:
 void reset() noexcept {ora_=orb_=ddra_=ddrb_=cra_=crb_=0;ira_=irb_=0;irqA_=irqB_=false;}
 std::uint8_t read(std::uint8_t offset) noexcept {
    switch(offset&3){
      case 0:if(!(cra_&4))return ddra_;irqA_=false;return std::uint8_t((ora_&ddra_)|(ira_&~ddra_));
      case 1:return std::uint8_t((cra_&0x3f)|(irqA_?0x80:0));
      case 2:if(!(crb_&4))return ddrb_;irqB_=false;return std::uint8_t((orb_&ddrb_)|(irb_&~ddrb_));
      default:return std::uint8_t((crb_&0x3f)|(irqB_?0x80:0));
    }
 }
 void write(std::uint8_t offset,std::uint8_t value) noexcept {
    switch(offset&3){
      case 0:if(cra_&4)ora_=value;else ddra_=value;break;
      case 1:cra_=value&0x3f;break;
      case 2:if(crb_&4)orb_=value;else ddrb_=value;break;
      default:crb_=value&0x3f;break;
    }
 }
 void setPortA(std::uint8_t value) noexcept {ira_=value;}
 void setPortB(std::uint8_t value) noexcept {irb_=value;}
 // IRQ latch remains asserted until the corresponding data port is read.
 void edgeCA1(bool rising) noexcept {if(rising==bool(cra_&2))irqA_=true;}
 void edgeCB1(bool rising) noexcept {if(rising==bool(crb_&2))irqB_=true;}
 void clearInterrupts() noexcept {irqA_=irqB_=false;}
 // Control register IRQ flags are status bits, not writable configuration.
 // An edge is latched regardless of whether IRQ output is enabled.
 bool irqA() const noexcept{return irqA_;}
 bool irqB() const noexcept{return irqB_;}
 bool irq() const noexcept{return (irqA_&&(cra_&1))||(irqB_&&(crb_&1));}
 std::uint8_t outputA() const noexcept{return ora_&ddra_;}
 std::uint8_t outputB() const noexcept{return orb_&ddrb_;}
private:
 std::uint8_t ora_=0,orb_=0,ddra_=0,ddrb_=0,cra_=0,crb_=0,ira_=0,irb_=0;
 bool irqA_=false,irqB_=false;
};
}
