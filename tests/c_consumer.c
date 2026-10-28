#include <meguviz/media.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
typedef DWORD ThreadId;
#define THREAD_ID GetCurrentThreadId()
#else
#include <pthread.h>
typedef pthread_t ThreadId;
#define THREAD_ID pthread_self()
#endif
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define INIT(v,t) do{memset(&(v),0,sizeof(v));(v).header=(MgcHeader)MGC_HEADER_INIT(t);}while(0)
typedef struct Context {int count,refs;ThreadId caller;MgmFrame *output;MgmEffect *effect;MgmEffectApi *api;} Context;
static void retain(void *p){++((Context*)p)->refs;}
static void release(void *p){--((Context*)p)->refs;}
static void completed(void *p,uint64_t request_id,MgcStatus status,MgmFrame *output){Context *c=(Context*)p;CHECK(THREAD_ID!=c->caller);CHECK(status==MGC_SUCCESS&&request_id==73&&output);CHECK(c->refs==1);CHECK(c->api->drain(c->effect,0)==MGC_BUSY);c->output=output;++c->count;}
int main(void){
    MgcRuntimeDesc rd;MgcRuntime *runtime=0;MgcDomainDesc dd;MgcDomain *domain=0;MgcResourceDesc bd;MgcResource *resource=0;MgcLeaseDesc ld;MgcLease *write=0,*read=0;MgmFrameDesc fd;MgmFrame *frame=0;MgmEffect *effect=0;MgmEffectApi api;MgmEffectRequest req;Context c={0};void *data=0;uint64_t size=0;
    INIT(rd,MgcRuntimeDesc);rd.provider=MGC_PROVIDER_CPU_REFERENCE;rd.memory_budget_bytes=4096;CHECK(mgc_runtime_create(&rd,&runtime)==MGC_SUCCESS);INIT(dd,MgcDomainDesc);CHECK(mgc_domain_acquire(runtime,&dd,&domain)==MGC_SUCCESS);
    INIT(bd,MgcResourceDesc);bd.allocation_id=17;bd.allocation_generation=1;bd.capacity_bytes=12;bd.width=2;bd.height=2;bd.row_stride_bytes=6;bd.format=MGC_FORMAT_RGB8;CHECK(mgc_resource_create(domain,&bd,&resource)==MGC_SUCCESS);
    INIT(ld,MgcLeaseDesc);ld.content_id=7;ld.byte_length=12;ld.access=MGC_ACCESS_WRITE;CHECK(mgc_lease_create(resource,&ld,&write)==MGC_SUCCESS);CHECK(mgc_lease_map(write,&data,&size)==MGC_SUCCESS&&size==12);memset(data,83,12);CHECK(mgc_lease_unmap(write)==MGC_SUCCESS);mgc_lease_release(write);ld.access=MGC_ACCESS_READ;CHECK(mgc_lease_create(resource,&ld,&read)==MGC_SUCCESS);
    INIT(fd,MgmFrameDesc);fd.content=read;fd.pts_ticks=45000;fd.timebase.numerator=1;fd.timebase.denominator=90000;fd.epoch=1;fd.crop_width=2;fd.crop_height=2;fd.frame_kind=MGM_FRAME_ORIGINAL;fd.color.header=(MgcHeader)MGC_HEADER_INIT(MgmColor);fd.color.transfer=MGM_TRANSFER_SDR;fd.color.bit_depth=8;fd.color.range=MGM_RANGE_FULL;CHECK(mgm_frame_create(&fd,&frame)==MGC_SUCCESS);
    INIT(api,MgmEffectApi);CHECK(mgm_identity_effect_create(domain,&effect,&api)==MGC_SUCCESS);INIT(req,MgmEffectRequest);req.request_id=73;req.epoch=1;req.left=frame;req.alpha=0;c.caller=THREAD_ID;c.effect=effect;c.api=&api;
    req.required_flags=1;CHECK(api.submit(effect,&req,completed,&c,retain,release)==MGC_UNSUPPORTED_ABI);req.required_flags=0;mgm_frame_retain(frame);CHECK(api.submit(effect,&req,completed,&c,retain,release)==MGC_ACCEPTED);CHECK(api.drain(effect,2000000000)==MGC_SUCCESS);CHECK(c.count==1&&c.refs==0);
    INIT(fd,MgmFrameDesc);CHECK(mgm_frame_describe(c.output,&fd)==MGC_SUCCESS&&fd.pts_ticks==45000&&fd.frame_kind==MGM_FRAME_ORIGINAL);
    CHECK(mgm_identity_effect_release(effect)==MGC_SUCCESS);mgm_frame_release(frame);mgc_lease_release(read);mgc_resource_release(resource);mgc_domain_release(domain);CHECK(mgc_runtime_close(runtime)==MGC_BUSY);CHECK(mgc_lease_map(fd.content,&data,&size)==MGC_SUCCESS&&size==12&&((unsigned char*)data)[11]==83);CHECK(mgc_lease_unmap(fd.content)==MGC_SUCCESS);mgm_frame_release(c.output);CHECK(mgc_runtime_close(runtime)==MGC_SUCCESS);mgc_runtime_release(runtime);
    puts("{\"result\":\"PASS\",\"consumer\":\"pure_c_identity\",\"pixels_copied\":false,\"callback\":\"worker_once\",\"consumer_output_survives_session\":true}");return 0;
}
