#include "ca_mame/compat/MameBridge.hpp"
#include <algorithm>
#include <cctype>
#include <istream>
#include <sstream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#endif
namespace ca_mame::compat {
bool MameBridge::validDriverName(std::string_view name) noexcept {
 return !name.empty() && name.size()<=64 && std::all_of(name.begin(),name.end(),[](unsigned char c){
  return std::isalnum(c)||c=='_';
 });
}
std::optional<LaunchPlan> MameBridge::plan(const std::filesystem::path& executable,std::string_view game,
 const std::optional<std::filesystem::path>& romPath){
 if(executable.empty()||!validDriverName(game))return std::nullopt;
 LaunchPlan result{executable,{std::string(game)}};
 if(romPath && !romPath->empty()){result.arguments.push_back("-rompath");result.arguments.push_back(romPath->string());}
 return result;
}
static std::string attribute(std::string_view tag,std::string_view key){
 const std::string needle=std::string(key)+"=\"";
 auto p=tag.find(needle);
 if(p==std::string_view::npos)return {};
 p+=needle.size();const auto end=tag.find('"',p);
 return end==std::string_view::npos?std::string{}:std::string(tag.substr(p,end-p));
}
static std::string element(std::string_view body,std::string_view name){
 const auto open=body.find("<"+std::string(name)+">");
 if(open==std::string_view::npos)return {};
 const auto start=open+name.size()+2;
 const auto end=body.find("</"+std::string(name)+">",start);
 return end==std::string_view::npos?std::string{}:std::string(body.substr(start,end-start));
}
std::vector<Game> MameBridge::parseListXml(std::string_view xml){
 std::vector<Game> result;
 std::size_t pos=0;
 while((pos=xml.find("<machine ",pos))!=std::string_view::npos){
  const auto end=xml.find('>',pos);
  if(end==std::string_view::npos)break;
  const auto close=xml.find("</machine>",end+1);
  if(close==std::string_view::npos)break;
  const auto tag=xml.substr(pos,end-pos+1),body=xml.substr(end+1,close-end-1);
  auto name=attribute(tag,"name");
  if(validDriverName(name)){
   Game game;game.name=std::move(name);game.description=element(body,"description");
   game.year=element(body,"year");game.manufacturer=element(body,"manufacturer");
   game.runnable=attribute(tag,"runnable")!="no";
   game.sourceFile=attribute(tag,"sourcefile");
   game.parent=attribute(tag,"cloneof");
   game.bios=attribute(tag,"romof");
   game.mechanical=attribute(tag,"ismechanical")=="yes";
   result.push_back(std::move(game));
  }
  pos=close+10;
 }
 return result;
}
std::vector<Game> MameBridge::parseListXml(std::istream& input){
 std::vector<Game> result;
 std::string pending,chunk(65536,'\0');
 // Incremental bounded-buffer scan; do not retain the full XML catalog.
 // The parser intentionally extracts only simple MAME machine metadata.
 while(input){
  input.read(chunk.data(),static_cast<std::streamsize>(chunk.size()));
  const auto count=input.gcount();
  if(count<=0)break;
  pending.append(chunk.data(),static_cast<std::size_t>(count));
  std::size_t consumed=0;
  for(;;){
   auto start=pending.find("<machine ",consumed);
   if(start==std::string::npos){
    if(consumed>0)pending.erase(0,consumed);
    if(pending.size()>128)pending.erase(0,pending.size()-128);
    break;
   }
   const auto headerEnd=pending.find('>',start);
   if(headerEnd==std::string::npos){pending.erase(0,start);break;}
   const auto close=pending.find("</machine>",headerEnd+1);
   if(close==std::string::npos){
    pending.erase(0,start);
    if(pending.size()>1024*1024)pending.clear(); // reject pathological entries
    break;
   }
   const auto tag=std::string_view(pending).substr(start,headerEnd-start+1);
   const auto body=std::string_view(pending).substr(headerEnd+1,close-headerEnd-1);
   auto name=attribute(tag,"name");
   if(validDriverName(name)){
    Game game;game.name=std::move(name);
    game.description=element(body,"description");
    game.year=element(body,"year");
    game.manufacturer=element(body,"manufacturer");
    game.runnable=attribute(tag,"runnable")!="no";
   game.sourceFile=attribute(tag,"sourcefile");
   game.parent=attribute(tag,"cloneof");
   game.bios=attribute(tag,"romof");
   game.mechanical=attribute(tag,"ismechanical")=="yes";
    result.push_back(std::move(game));
   }
   consumed=close+10;
  }
 }
 return result;
}
std::optional<LaunchPlan> MameBridge::verifyRomsPlan(const std::filesystem::path& executable,
 std::string_view game,const std::optional<std::filesystem::path>& romPath){
 auto p=plan(executable,game,romPath);
 if(!p)return std::nullopt;
 p->arguments.insert(p->arguments.begin(),"-verifyroms");
 return p;
}
int MameBridge::launch(const LaunchPlan& p){
 if(p.executable.empty())return -1;
#ifdef _WIN32
 auto quote=[](const std::wstring& a){
  std::wstring out=L"\"";std::size_t slashes=0;
  for(wchar_t c:a){
   if(c==L'\\'){++slashes;continue;}
   if(c==L'"'){out.append(slashes*2+1,L'\\');out+=L'"';slashes=0;continue;}
   out.append(slashes,L'\\');slashes=0;out+=c;
  }
  out.append(slashes*2,L'\\');out+=L'"';return out;
 };
 std::wstring cmd=quote(p.executable.wstring());
 for(const auto& a:p.arguments){cmd+=L" ";cmd+=quote(std::filesystem::path(a).wstring());}
 std::vector<wchar_t> mutableCommand(cmd.begin(),cmd.end());mutableCommand.push_back(0);
 STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
 const auto exe=p.executable.wstring();
 if(!CreateProcessW(exe.c_str(),mutableCommand.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&process))return -1;
 WaitForSingleObject(process.hProcess,INFINITE);
 DWORD exitCode=0;const bool ok=GetExitCodeProcess(process.hProcess,&exitCode)!=0;
 CloseHandle(process.hThread);CloseHandle(process.hProcess);
 return ok?static_cast<int>(exitCode):-1;
#else
 std::vector<std::string> strings;strings.reserve(p.arguments.size()+1);
 strings.push_back(p.executable.string());
 strings.insert(strings.end(),p.arguments.begin(),p.arguments.end());
 std::vector<char*> argv;argv.reserve(strings.size()+1);
 for(auto& str:strings)argv.push_back(str.data());
 argv.push_back(nullptr);
 const pid_t pid=fork();
 if(pid<0)return -1;
 if(pid==0){execvp(argv[0],argv.data());_exit(127);}
 int status=0;
 while(waitpid(pid,&status,0)<0){if(errno!=EINTR)return -1;}
 return WIFEXITED(status)?WEXITSTATUS(status):-1;
#endif
}

ProcessResult MameBridge::capture(const LaunchPlan& p,std::size_t maxBytes){
 ProcessResult result;
 if(p.executable.empty())return result;
#ifdef _WIN32
 SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
 HANDLE readPipe=nullptr,writePipe=nullptr;
 if(!CreatePipe(&readPipe,&writePipe,&security,0))return result;
 SetHandleInformation(readPipe,HANDLE_FLAG_INHERIT,0);
 auto quote=[](const std::wstring& a){
  std::wstring out=L"\"";std::size_t slashes=0;
  for(wchar_t c:a){
   if(c==L'\\'){++slashes;continue;}
   if(c==L'"'){out.append(slashes*2+1,L'\\');out+=L'"';slashes=0;continue;}
   out.append(slashes,L'\\');slashes=0;out+=c;
  }
  out.append(slashes*2,L'\\');out+=L'"';return out;
 };
 std::wstring command=quote(p.executable.wstring());
 for(const auto& arg:p.arguments)command+=L" "+quote(std::filesystem::path(arg).wstring());
 std::vector<wchar_t> mutableCommand(command.begin(),command.end());mutableCommand.push_back(0);
 STARTUPINFOW startup{};startup.cb=sizeof(startup);
 startup.dwFlags=STARTF_USESTDHANDLES;
 startup.hStdOutput=writePipe;startup.hStdError=writePipe;
 startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
 PROCESS_INFORMATION process{};
 const auto exe=p.executable.wstring();
 const BOOL started=CreateProcessW(exe.c_str(),mutableCommand.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process);
 CloseHandle(writePipe);
 if(!started){CloseHandle(readPipe);return result;}
 char buffer[4096];DWORD count=0;
 while(ReadFile(readPipe,buffer,sizeof(buffer),&count,nullptr)&&count){
  const auto room=maxBytes>result.output.size()?maxBytes-result.output.size():0;
  result.output.append(buffer,std::min<std::size_t>(room,count));
  if(count>room)result.truncated=true;
 }
 CloseHandle(readPipe);
 WaitForSingleObject(process.hProcess,INFINITE);
 DWORD code=0;
 if(GetExitCodeProcess(process.hProcess,&code))result.exitCode=static_cast<int>(code);
 CloseHandle(process.hThread);CloseHandle(process.hProcess);
#else
 int pipes[2];
 if(pipe(pipes)!=0)return result;
 std::vector<std::string> args;args.reserve(p.arguments.size()+1);
 args.push_back(p.executable.string());
 args.insert(args.end(),p.arguments.begin(),p.arguments.end());
 std::vector<char*> argv;argv.reserve(args.size()+1);
 for(auto& a:args)argv.push_back(a.data());
 argv.push_back(nullptr);
 const pid_t pid=fork();
 if(pid<0){close(pipes[0]);close(pipes[1]);return result;}
 if(pid==0){
  close(pipes[0]);
  if(dup2(pipes[1],STDOUT_FILENO)<0||dup2(pipes[1],STDERR_FILENO)<0)_exit(127);
  close(pipes[1]);execvp(argv[0],argv.data());_exit(127);
 }
 close(pipes[1]);
 char buffer[4096];ssize_t count=0;
 while((count=read(pipes[0],buffer,sizeof(buffer)))>0){
  const auto room=maxBytes>result.output.size()?maxBytes-result.output.size():0;
  result.output.append(buffer,std::min<std::size_t>(room,static_cast<std::size_t>(count)));
  if(static_cast<std::size_t>(count)>room)result.truncated=true;
 }
 close(pipes[0]);
 int status=0;
 while(waitpid(pid,&status,0)<0){if(errno!=EINTR)return result;}
 if(WIFEXITED(status))result.exitCode=WEXITSTATUS(status);
#endif
 return result;
}

}
