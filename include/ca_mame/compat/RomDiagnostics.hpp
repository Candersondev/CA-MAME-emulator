#pragma once
#include <string>
#include <string_view>
#include <vector>
namespace ca_mame::compat {
enum class RomIssueKind { Missing, Incorrect, Checksum, Other };
struct RomIssue { RomIssueKind kind; std::string detail; };
struct RomDiagnostics { std::vector<RomIssue> issues; bool passed=false; bool truncated=false; };
RomDiagnostics analyzeRomVerification(std::string_view output,int exitCode,bool truncated=false);
std::string summarizeRomVerification(const RomDiagnostics& report);
}
