#ifndef MEGUVIZ_DESCRIPTOR_PAGES_H
#define MEGUVIZ_DESCRIPTOR_PAGES_H
#include <meguviz/compute.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#ifndef CHECK
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<__LINE__<<" "<<#x<<std::endl;std::exit(1);}}while(0)
#endif
/* Exactly size accessible bytes precede an inaccessible page. No full-size
 * descriptor is allocated behind an old-prefix or header-only caller. */
struct DescriptorPages {
    void *base=nullptr,*value=nullptr;size_t page=0,size=0;
    explicit DescriptorPages(size_t bytes=sizeof(MgcHeader)):size(bytes){
#ifdef _WIN32
        SYSTEM_INFO info{};GetSystemInfo(&info);page=info.dwPageSize;
        base=VirtualAlloc(nullptr,page*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(base);
        DWORD old;CHECK(VirtualProtect(static_cast<char*>(base)+page,page,PAGE_NOACCESS,&old));
#else
        page=size_t(sysconf(_SC_PAGESIZE));base=mmap(nullptr,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);CHECK(base!=MAP_FAILED);
        CHECK(mprotect(static_cast<char*>(base)+page,page,PROT_NONE)==0);
#endif
        CHECK(size>=sizeof(MgcHeader)&&size<=page);value=static_cast<char*>(base)+page-size;
        std::memset(value,0,size);MgcHeader h{uint32_t(size),MGC_ABI_MAJOR,MGC_ABI_MINOR};std::memcpy(value,&h,sizeof(h));
    }
    ~DescriptorPages(){
#ifdef _WIN32
        VirtualFree(base,0,MEM_RELEASE);
#else
        munmap(base,page*2);
#endif
    }
    DescriptorPages(const DescriptorPages&)=delete;DescriptorPages& operator=(const DescriptorPages&)=delete;
    template<class T>T *as(){return static_cast<T*>(value);}
    template<class T>void store(T copy){copy.header.struct_size=uint32_t(size);std::memcpy(value,&copy,size<sizeof(T)?size:sizeof(T));}
    template<class T>T read()const{T copy{};std::memcpy(&copy,value,size<sizeof(T)?size:sizeof(T));return copy;}
};
#endif
