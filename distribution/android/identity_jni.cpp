#include <jni.h>
#include <meguviz/media.h>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <vector>

namespace {
struct Context {std::mutex mutex;std::condition_variable changed;MgcStatus status=MGC_INTERNAL_ERROR;MgmFrame *output=nullptr;bool done=false;};
void complete(void *p,uint64_t,MgcStatus status,MgmFrame *output){auto c=static_cast<Context*>(p);std::lock_guard<std::mutex> lock(c->mutex);c->status=status;c->output=output;c->done=true;c->changed.notify_one();}
struct Owner {
    MgcRuntime *runtime=nullptr;MgcDomain *domain=nullptr;MgcResource *resource=nullptr;MgcLease *lease=nullptr;MgmFrame *frame=nullptr;MgmEffect *effect=nullptr;MgmEffectApi api{};Context context;
    ~Owner(){if(effect){api.drain(effect,UINT64_MAX);mgm_identity_effect_release(effect);}mgm_frame_release(context.output);mgm_frame_release(frame);mgc_lease_release(lease);mgc_resource_release(resource);mgc_domain_release(domain);if(runtime){mgc_runtime_close(runtime);mgc_runtime_release(runtime);}}
};
void fail(JNIEnv *env,const char *message){auto cls=env->FindClass("java/lang/IllegalArgumentException");if(cls)env->ThrowNew(cls,message);}
}
extern "C" JNIEXPORT jint JNICALL Java_io_github_jichuo1_meguviz_media_MediaNative_abiVersion(JNIEnv*,jclass){return 1;}
extern "C" JNIEXPORT jbyteArray JNICALL Java_io_github_jichuo1_meguviz_media_MediaNative_identityRgb(JNIEnv *env,jclass,jbyteArray input,jint width,jint height,jint stride){
    if(!input||width<=0||height<=0||width>4096||height>4096||stride<width*3||uint64_t(stride)*uint64_t(height)>64*1024*1024){fail(env,"Invalid RGB shape or stride");return nullptr;}
    const uint64_t bytes=uint64_t(stride)*height;if(bytes>uint64_t(env->GetArrayLength(input))){fail(env,"RGB array is shorter than the declared image");return nullptr;}
    try {
        Owner owner;MgcRuntimeDesc rd{};rd.header=MGC_HEADER_INIT(MgcRuntimeDesc);rd.provider=MGC_PROVIDER_CPU_REFERENCE;rd.memory_budget_bytes=bytes;auto status=mgc_runtime_create(&rd,&owner.runtime);if(status){fail(env,"Cannot create CPU reference runtime");return nullptr;}
        MgcDomainDesc dd{};dd.header=MGC_HEADER_INIT(MgcDomainDesc);status=mgc_domain_acquire(owner.runtime,&dd,&owner.domain);if(status){fail(env,"Cannot create reference domain");return nullptr;}
        MgcResourceDesc resource{};resource.header=MGC_HEADER_INIT(MgcResourceDesc);resource.capacity_bytes=bytes;resource.format=MGC_FORMAT_RGB8;resource.width=uint32_t(width);resource.height=uint32_t(height);resource.row_stride_bytes=uint64_t(stride);status=mgc_resource_create(owner.domain,&resource,&owner.resource);if(status){fail(env,"Cannot allocate reference image");return nullptr;}
        MgcLeaseDesc ld{};ld.header=MGC_HEADER_INIT(MgcLeaseDesc);ld.content_id=1;ld.byte_length=bytes;ld.access=MGC_ACCESS_WRITE;status=mgc_lease_create(owner.resource,&ld,&owner.lease);if(status){fail(env,"Cannot acquire image content");return nullptr;}
        void *data=nullptr;uint64_t capacity=0;status=mgc_lease_map(owner.lease,&data,&capacity);if(status){fail(env,"Cannot map reference image");return nullptr;}env->GetByteArrayRegion(input,0,jsize(bytes),static_cast<jbyte*>(data));mgc_lease_unmap(owner.lease);if(env->ExceptionCheck())return nullptr;mgc_lease_release(owner.lease);owner.lease=nullptr;ld.access=MGC_ACCESS_READ;status=mgc_lease_create(owner.resource,&ld,&owner.lease);if(status){fail(env,"Cannot read reference image");return nullptr;}
        MgmFrameDesc frame{};frame.header=MGC_HEADER_INIT(MgmFrameDesc);frame.content=owner.lease;frame.epoch=1;frame.timebase={1,1000000};frame.crop_width=uint32_t(width);frame.crop_height=uint32_t(height);frame.frame_kind=MGM_FRAME_ORIGINAL;frame.color.header=MGC_HEADER_INIT(MgmColor);frame.color.transfer=MGM_TRANSFER_SDR;frame.color.range=MGM_RANGE_FULL;frame.color.bit_depth=8;status=mgm_frame_create(&frame,&owner.frame);if(status){fail(env,"Cannot create reference frame");return nullptr;}
        owner.api.header=MGC_HEADER_INIT(MgmEffectApi);status=mgm_identity_effect_create(owner.domain,&owner.effect,&owner.api);if(status){fail(env,"Cannot create identity effect");return nullptr;}
        MgmEffectRequest request{};request.header=MGC_HEADER_INIT(MgmEffectRequest);request.request_id=1;request.epoch=1;request.left=owner.frame;mgm_frame_retain(owner.frame);status=owner.api.submit(owner.effect,&request,complete,&owner.context,nullptr,nullptr);if(status!=MGC_ACCEPTED){mgm_frame_release(owner.frame);fail(env,"Identity submission was rejected");return nullptr;}
        status=owner.api.drain(owner.effect,UINT64_MAX);if(status||!owner.context.done||owner.context.status){fail(env,"Identity execution failed");return nullptr;}
        MgmFrameDesc result{};result.header=MGC_HEADER_INIT(MgmFrameDesc);status=mgm_frame_describe(owner.context.output,&result);if(status){fail(env,"Missing identity output");return nullptr;}status=mgc_lease_map(result.content,&data,&capacity);if(status||capacity!=bytes){if(!status)mgc_lease_unmap(result.content);fail(env,"Invalid identity output");return nullptr;}
        auto output=env->NewByteArray(jsize(bytes));if(output)env->SetByteArrayRegion(output,0,jsize(bytes),static_cast<const jbyte*>(data));mgc_lease_unmap(result.content);return output;
    }catch(...){if(!env->ExceptionCheck())fail(env,"Reference processing could not complete");return nullptr;}
}
