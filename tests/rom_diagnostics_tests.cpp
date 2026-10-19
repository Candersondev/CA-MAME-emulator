#include "ca_mame/compat/RomDiagnostics.hpp"
#include <string>
int main(){
 using namespace ca_mame::compat;
 auto report=analyzeRomVerification("abc.bin NOT FOUND\r\nfoo.bin WRONG LENGTH\nbar.bin BAD CRC\n",1);
 if(report.passed||report.issues.size()!=3)return 1;
 if(report.issues[0].kind!=RomIssueKind::Missing||report.issues[1].kind!=RomIssueKind::Incorrect||report.issues[2].kind!=RomIssueKind::Checksum)return 2;
 if(summarizeRomVerification(report)!="FAIL: 1 missing, 1 incorrect, 1 checksum issues")return 3;
 if(!analyzeRomVerification("romset is good",0).passed)return 4;
 if(analyzeRomVerification("unknown failure",1).issues.size()!=0)return 5;
 return 0;
}
