#include "ca_mame/compat/AuditStore.hpp"
#include <chrono>
#include <filesystem>
int main(){
 namespace fs=std::filesystem;
 using namespace ca_mame::compat;
 const auto path=fs::temp_directory_path()/("ca_mame_audit_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".tsv");
 struct Cleanup{fs::path path;~Cleanup(){std::error_code ec;fs::remove(path,ec);fs::remove(path.string()+".tmp",ec);}} cleanup{path};
 AuditStore a;
 if(a.status("evilngt")!=AuditState::Unknown)return 1;
 a.record("evilngt",0);a.record("invaders",1);a.record("../invalid",0);a.record("missing",-1);
 if(a.size()!=2||a.status("evilngt")!=AuditState::Passed||a.status("invaders")!=AuditState::Failed)return 2;
 if(!a.save(path))return 3;
 AuditStore b;if(!b.load(path)||b.size()!=2||b.status("evilngt")!=AuditState::Passed)return 4;
 b.record("evilngt",1);if(!b.save(path))return 5;
 AuditStore c;if(!c.load(path)||c.status("evilngt")!=AuditState::Failed)return 6;
 return 0;
}
