#include "ca_mame/compat/AuditStore.hpp"
#include "ca_mame/compat/MameBridge.hpp"
#include <fstream>
#include <system_error>
namespace ca_mame::compat {
bool AuditStore::load(const std::filesystem::path& file){
 std::ifstream in(file,std::ios::binary);
 if(!in)return false;
 std::map<std::string,AuditState> parsed;
 std::string line;
 if(!std::getline(in,line)||line!="CA-MAME-AUDIT-V1")return false;
 while(std::getline(in,line)){
  const auto tab=line.find('\t');
  if(tab==std::string::npos)return false;
  const auto id=line.substr(0,tab),state=line.substr(tab+1);
  if(!MameBridge::validDriverName(id)||(state!="PASS"&&state!="FAIL"))return false;
  parsed[id]=state=="PASS"?AuditState::Passed:AuditState::Failed;
 }
 if(in.bad())return false;
 results_=std::move(parsed);
 return true;
}
bool AuditStore::save(const std::filesystem::path& file) const{
 const auto temp=std::filesystem::path(file.string()+".tmp");
 {
  std::ofstream out(temp,std::ios::binary|std::ios::trunc);
  if(!out)return false;
  out<<"CA-MAME-AUDIT-V1\n";
  for(const auto& [id,state]:results_)
   if(state!=AuditState::Unknown)out<<id<<'\t'<<(state==AuditState::Passed?"PASS":"FAIL")<<'\n';
  out.flush();
  if(!out){out.close();std::error_code ec;std::filesystem::remove(temp,ec);return false;}
 }
 std::error_code ec;
 std::filesystem::rename(temp,file,ec);
 if(!ec)return true;
 // Windows does not replace existing files via rename.
 std::filesystem::remove(file,ec);
 ec.clear();std::filesystem::rename(temp,file,ec);
 return !ec;
}
void AuditStore::record(std::string_view driver,int exitCode){
 if(!MameBridge::validDriverName(driver))return;
 if(exitCode>=0)results_[std::string(driver)]=exitCode==0?AuditState::Passed:AuditState::Failed;
}
AuditState AuditStore::status(std::string_view driver) const{
 const auto it=results_.find(std::string(driver));
 return it==results_.end()?AuditState::Unknown:it->second;
}
}
