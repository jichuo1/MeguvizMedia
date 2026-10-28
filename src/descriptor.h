#ifndef MEGUVIZ_MEDIA_DESCRIPTOR_H
#define MEGUVIZ_MEDIA_DESCRIPTOR_H
#include <meguviz/compute.h>
#include <cstring>

namespace mgm_detail {
inline MgcStatus read_header(const void *source,size_t minimum,MgcHeader &header){
    if(!source)return MGC_INVALID_ARGUMENT;
    std::memcpy(&header,source,sizeof(header));
    if(header.struct_size<minimum)return MGC_INVALID_ARGUMENT;
    return header.abi_major==MGC_ABI_MAJOR?MGC_SUCCESS:MGC_UNSUPPORTED_ABI;
}
template<class T>MgcStatus copy_desc(const T *source,size_t minimum,T &copy,uint32_t *size=nullptr){
    MgcHeader header{};auto status=read_header(source,minimum,header);if(status)return status;
    copy={};std::memcpy(&copy,source,header.struct_size<sizeof(T)?header.struct_size:sizeof(T));
    if(size)*size=header.struct_size;
    copy.header.struct_size=sizeof(T);return MGC_SUCCESS;
}
template<class T>void write_desc(T *destination,T copy,uint32_t size){
    copy.header.struct_size=size;
    std::memcpy(destination,&copy,size<sizeof(T)?size:sizeof(T));
}
template<class T,size_t N>bool zero(const T (&values)[N]){for(auto value:values)if(value)return false;return true;}
}
#endif
