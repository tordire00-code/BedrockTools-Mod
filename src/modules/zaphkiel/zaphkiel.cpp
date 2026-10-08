#include "zaphkiel.hpp"
#include <thread>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"Zaphkiel",__VA_ARGS__)

static ZaphkielModule* g=nullptr;
static std::string readF(std::string p){ std::ifstream f(p); if(!f) return ""; std::stringstream ss; ss<<f.rdbuf(); return ss.str(); }
static void writeF(std::string p,std::string d){ std::filesystem::create_directories(std::filesystem::path(p).parent_path()); std::ofstream f(p); f<<d; }
static std::string demangle_c(const char* m){ int s=0; char* d=abi::__cxa_demangle(m,0,0,&s); std::string r=s==0&&d?d:m; if(d) free(d); return r; }
struct LibInfo{ uintptr_t base=0; std::string path; uintptr_t rodata_off=0; size_t rodata_sz=0; };
static LibInfo getLib(const char* name){
    LibInfo li; std::ifstream maps("/proc/self/maps"); std::string line;
    while(std::getline(maps,line)){ if(line.find(name)!=std::string::npos){
        uintptr_t b=0; char perms[8]={0}, path[512]={0}; sscanf(line.c_str(),"%lx-%*x %7s %*s %*s %*s %511s",&b,perms,path);
        if(li.base==0){ li.base=b; li.path=path; }
    }}
    if(!li.path.empty()){
        int fd=open(li.path.c_str(),O_RDONLY); if(fd>=0){
            struct stat st; fstat(fd,&st);
            void* map=mmap(0,st.st_size,PROT_READ,MAP_PRIVATE,fd,0);
            if(map!=MAP_FAILED){
                auto *eh=(Elf64_Ehdr*)map; auto *sh=(Elf64_Shdr*)((uint8_t*)map+eh->e_shoff);
                const char* shstr=(char*)map+sh[eh->e_shstrndx].sh_offset;
                for(int i=0;i<eh->e_shnum;i++){ std::string n=shstr+sh[i].sh_name; if(n==".rodata"){ li.rodata_off=sh[i].sh_offset; li.rodata_sz=sh[i].sh_size; break; } }
                munmap(map,st.st_size);
            } close(fd);
        }
    }
    return li;
}
void ZaphkielModule::ensureFolders(){ std::filesystem::create_directories(mediaBase); std::filesystem::create_directories(mediaBase+"/hooks"); std::filesystem::create_directories(mediaBase+"/sigs"); std::filesystem::create_directories(mediaBase+"/dump"); }
void ZaphkielModule::loadAll(){
    ensureFolders();
    auto txt=readF(cachePath); entries.clear();
    if(!txt.empty()){
        std::stringstream ss(txt); std::string l;
        while(std::getline(ss,l)){ if(l.empty()) continue; char name[256]; char addr_s[32]; int vs=0;
            if(sscanf(l.c_str(),"%255s %31s %d",name,addr_s,&vs)>=1){ DumpEntry e; e.name=name; e.addr=strtoull(addr_s,nullptr,16); e.vsize=vs; entries.push_back(e); }
        }
        status="Loaded: "+std::to_string(entries.size()); prog=1.0f; found=entries.size();
    }
}
void ZaphkielModule::fastScan(){
    if(running) return; running=true; prog=0.05f; found=0; status="FAST SCAN 2s...";
    std::thread([this]{
        auto lib=getLib("libminecraftpe.so");
        if(!lib.base || lib.rodata_sz==0){ status="lib not found"; running=false; return; }
        uint8_t* rodata_mem=(uint8_t*)(lib.base+lib.rodata_off); size_t sz=lib.rodata_sz;
        std::vector<DumpEntry> tmp;
        for(size_t i=0;i+8<sz;){ void* p=memchr(rodata_mem+i,'_',sz-i); if(!p) break; size_t off=(uint8_t*)p-rodata_mem;
            if(off+6<sz && memcmp(p,"_ZTS",4)==0){ const char* s=(const char*)p+4; int len=0; const char* t=s;
                while(t<(char*)rodata_mem+sz && isdigit(*t) && len<6){ len=len*10+(*t-'0'); t++; }
                if(len>1 && len<200 && t+len < (char*)rodata_mem+sz){ char buf[256]={0}; memcpy(buf,t,len); std::string dem=demangle_c(buf);
                    if(dem.size()>2 && dem.size()<100){ tmp.push_back({dem, lib.base+off, 16}); found++; } i=off+4+len;
                } else i=off+1;
            } else i=off+1; prog=0.1f+0.85f*float(i)/float(sz);
        }
        std::string hexout; for(auto &e:tmp){ char h[512]; snprintf(h,512,"%s %lx %d\n",e.name.c_str(),e.addr,e.vsize); hexout+=h; }
        writeF(cachePath,hexout); writeF(mediaBase+"/dump/all_classes.txt",hexout);
        entries=tmp; prog=1.0f; status="Done! "+std::to_string(entries.size())+" -> "+mediaBase; running=false;
    }).detach();
}
void ZaphkielModule::genHook(){ if(sel<0) return; auto &e=entries[sel]; char code[2048]; snprintf(code,2048,"// %s\n// Zaphkiel hook -> %s/hooks/%s.cpp\nvoid** vt=(void**)(0x%lx);\nvoid* target=vt[%d];\n// DobbyHook(target, my_%s, &orig_%s);\n",e.name.c_str(),mediaBase.c_str(),e.name.c_str(),e.addr,methodIdx,e.name.c_str(),e.name.c_str()); hookCode=code; writeF(mediaBase+"/hooks/"+e.name+".cpp",code); status="Hook saved: "+e.name; }
void ZaphkielModule::genSig(){ if(sel<0) return; auto &e=entries[sel]; sigCode="CLASS: "+e.name+"\nADDR: 0x"+std::to_string(e.addr)+"\nSIG:?\nFILE: "+mediaBase+"/sigs/"+e.name+".sig"; writeF(mediaBase+"/sigs/"+e.name+".sig",sigCode); status="Sig saved: "+e.name; }
void ZaphkielModule::onInit(){ g=this; ensureFolders(); loadAll(); if(entries.empty()) fastScan(); }
void ZaphkielModule::onMenuRegistered(){}
bool ZaphkielModule::onMenuConfigChanged(std::string_view k, std::string_view v){ return false; }
void ZaphkielModule::onFrame(){}
