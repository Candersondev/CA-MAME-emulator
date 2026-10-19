#include "ca_mame/compat/RomDiagnostics.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
namespace ca_mame::compat {
static std::string lower(std::string s){
 std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
 return s;
}
RomDiagnostics analyzeRomVerification(std::string_view output,int exitCode,bool truncated){
 RomDiagnostics report;report.passed=exitCode==0;report.truncated=truncated;
 std::istringstream input{std::string(output)};std::string line;
 while(std::getline(input,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  const auto value=lower(line);
  RomIssueKind kind=RomIssueKind::Other;
  if(value.find("not found")!=std::string::npos||value.find("missing")!=std::string::npos)kind=RomIssueKind::Missing;
  else if(value.find("incorrect")!=std::string::npos||value.find("wrong length")!=std::string::npos||value.find("bad length")!=std::string::npos)kind=RomIssueKind::Incorrect;
  else if(value.find("checksum")!=std::string::npos||value.find("bad crc")!=std::string::npos||value.find("bad sha1")!=std::string::npos)kind=RomIssueKind::Checksum;
  else continue;
  if(report.issues.size()<100)report.issues.push_back({kind,line});
 }
 return report;
}
std::string summarizeRomVerification(const RomDiagnostics& report){
 if(report.passed)return "PASS: ROM verification succeeded";
 std::size_t missing=0,incorrect=0,checksum=0;
 for(const auto& issue:report.issues){
  if(issue.kind==RomIssueKind::Missing)++missing;
  else if(issue.kind==RomIssueKind::Incorrect)++incorrect;
  else if(issue.kind==RomIssueKind::Checksum)++checksum;
 }
 if(report.issues.empty())return "FAIL: see MAME output for details";
 return "FAIL: "+std::to_string(missing)+" missing, "+std::to_string(incorrect)+" incorrect, "+std::to_string(checksum)+" checksum issues"+(report.truncated?" (partial output)":"");
}
}
