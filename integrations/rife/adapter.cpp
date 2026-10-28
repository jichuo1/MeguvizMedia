#include <meguviz/media_rife.h>
#include "../../src/descriptor.h"
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <new>

namespace {
struct Effect {MgrSession *session=nullptr;std::mutex mutex;bool busy=false,cancelled=false,closing=false;uint64_t request_id=0;MgcTicket *ticket=nullptr;};
struct Job {std::atomic<uint32_t> refs{1};Effect *effect;MgmEffectRequest request;MgmFrameDesc left,right;MgmEffectCompletionFn callback;void *context;MgcContextFn retain_context,release_context;};
Effect *effect(MgmEffect *e){return reinterpret_cast<Effect*>(e);}
using mgm_detail::copy_desc;
using mgm_detail::write_desc;
using mgm_detail::zero;
void retain_job(void *p){auto j=static_cast<Job*>(p);++j->refs;if(j->retain_context)j->retain_context(j->context);}
void drop_job(Job *j){if(--j->refs==0){mgm_frame_release(j->request.left);mgm_frame_release(j->request.right);delete j;}}
void release_job(void *p){auto j=static_cast<Job*>(p);if(j->release_context)j->release_context(j->context);drop_job(j);}
void completed(void *p,uint64_t id,MgcStatus status,MgrResult *result){auto j=static_cast<Job*>(p);MgmFrame *output=nullptr;
    if(status==MGC_SUCCESS&&result){MgrResultInfo info{};info.header=MGC_HEADER_INIT(MgrResultInfo);status=mgr_result_describe(result,&info);if(status==MGC_SUCCESS&&(!info.algorithm_accepted||!info.output))status=MGC_INTERNAL_ERROR;if(status==MGC_SUCCESS){auto desc=j->left;desc.content=info.output;const long double pts=static_cast<long double>(j->left.pts_ticks)+(static_cast<long double>(j->right.pts_ticks)-j->left.pts_ticks)*j->request.alpha;if(pts<0||pts>=static_cast<long double>(INT64_MAX))status=MGC_INVALID_ARGUMENT;else{desc.pts_ticks=int64_t(std::floor(pts+0.5L));desc.frame_kind=info.frame_kind==MGR_RESULT_INFERRED?MGM_FRAME_INFERRED:(info.frame_kind==MGR_RESULT_ENDPOINT?MGM_FRAME_ENDPOINT:MGM_FRAME_BYPASS);status=mgm_frame_create(&desc,&output);}}}
    if(result)mgr_result_release(result);j->callback(j->context,id,status,output);
    MgcTicket *ticket=nullptr;{std::lock_guard<std::mutex> lock(j->effect->mutex);ticket=j->effect->ticket;j->effect->ticket=nullptr;j->effect->busy=false;}if(ticket)mgc_ticket_release(ticket);
}
MgcStatus query(MgmEffect *e,MgcCapability *c){if(!e)return MGC_INVALID_ARGUMENT;MgcCapability value{};uint32_t size=0;auto status=copy_desc(c,offsetof(MgcCapability,reserved),value,&size);if(status)return status;if(!zero(value.reserved))return MGC_INVALID_ARGUMENT;value.feature=MGC_CAP_CPU_REFERENCE;value.advertised=1;value.enabled=1;value.probe_result=MGC_SUCCESS;value.workload_qualified=0;value.reason=MGC_SUCCESS;write_desc(c,value,size);return MGC_SUCCESS;}
MgcStatus prepare(MgmEffect *e,const MgmFrameDesc *d){if(!e)return MGC_INVALID_ARGUMENT;MgmFrameDesc copied{};auto status=copy_desc(d,offsetof(MgmFrameDesc,reserved),copied);if(status)return status;d=&copied;if(d->required_flags)return MGC_UNSUPPORTED_ABI;for(auto v:d->reserved)if(v)return MGC_INVALID_ARGUMENT;if(d->color.transfer!=MGM_TRANSFER_SDR||d->color.bit_depth!=8)return MGC_UNSUPPORTED_CAPABILITY;return MGC_SUCCESS;}
MgcStatus submit(MgmEffect *opaque,const MgmEffectRequest *r,MgmEffectCompletionFn callback,void *context,MgcContextFn retain,MgcContextFn release){if(!opaque||!callback)return MGC_INVALID_ARGUMENT;MgmEffectRequest copied{};auto status=copy_desc(r,offsetof(MgmEffectRequest,reserved),copied);if(status)return status;r=&copied;if(r->required_flags)return MGC_UNSUPPORTED_ABI;for(auto v:r->reserved)if(v)return MGC_INVALID_ARGUMENT;if(!r->left||!r->right||!std::isfinite(r->alpha)||r->alpha<0||r->alpha>1||!!retain!=!!release)return MGC_INVALID_ARGUMENT;auto e=effect(opaque);
    MgmFrameDesc a{},b{};a.header=MGC_HEADER_INIT(MgmFrameDesc);b.header=MGC_HEADER_INIT(MgmFrameDesc);status=mgm_frame_describe(r->left,&a);if(status)return status;status=mgm_frame_describe(r->right,&b);if(status)return status;if(a.epoch!=r->epoch||b.epoch!=r->epoch||a.timebase.numerator!=b.timebase.numerator||a.timebase.denominator!=b.timebase.denominator)return MGC_INVALID_ARGUMENT;if(prepare(opaque,&a)||prepare(opaque,&b))return MGC_UNSUPPORTED_CAPABILITY;
    if(a.color.primaries!=b.color.primaries||a.color.transfer!=b.color.transfer||a.color.range!=b.color.range||a.color.matrix!=b.color.matrix||a.color.chroma_siting!=b.color.chroma_siting||a.color.bit_depth!=b.color.bit_depth||a.color.premultiplied!=b.color.premultiplied||a.rotation_degrees!=b.rotation_degrees||a.crop_left!=b.crop_left||a.crop_top!=b.crop_top||a.crop_width!=b.crop_width||a.crop_height!=b.crop_height)return MGC_UNSUPPORTED_CAPABILITY;
    MgcLeaseInfo info{};info.header=MGC_HEADER_INIT(MgcLeaseInfo);status=mgc_lease_info(a.content,&info);if(status)return status;
    auto job=new(std::nothrow)Job;if(!job)return MGC_OUT_OF_MEMORY;job->effect=e;job->request=*r;job->left=a;job->right=b;job->callback=callback;job->context=context;job->retain_context=retain;job->release_context=release;mgm_frame_retain(r->left);mgm_frame_retain(r->right);
    {std::lock_guard<std::mutex> lock(e->mutex);if(e->busy||e->closing){drop_job(job);return e->closing?MGC_BUSY:MGC_BACKPRESSURE;}e->busy=true;e->cancelled=false;e->request_id=r->request_id;}
    MgrRequest request{};request.header=MGC_HEADER_INIT(MgrRequest);request.request_id=r->request_id;request.left=a.content;request.right=b.content;request.alpha=r->alpha;request.output.header=MGC_HEADER_INIT(MgrOutputSpec);request.output.width=info.width;request.output.height=info.height;request.output.format=info.format;request.output.row_stride_bytes=uint64_t(info.width)*3;
    mgc_lease_retain(a.content);mgc_lease_retain(b.content);MgcTicket *ticket=nullptr;status=mgr_submit(e->session,&request,completed,job,retain_job,release_job,&ticket);
    if(status==MGC_ACCEPTED){mgm_frame_release(r->left);mgm_frame_release(r->right);bool cancel_now=false,finished=false;{std::lock_guard<std::mutex> lock(e->mutex);finished=!e->busy;if(!finished){e->ticket=ticket;cancel_now=e->cancelled;if(cancel_now)mgc_ticket_retain(ticket);}}if(finished)mgc_ticket_release(ticket);else if(cancel_now){mgr_cancel(e->session,ticket);mgc_ticket_release(ticket);}}else{mgc_lease_release(a.content);mgc_lease_release(b.content);std::lock_guard<std::mutex> lock(e->mutex);e->busy=false;}
    drop_job(job);return status;
}
MgcStatus cancel(MgmEffect *opaque,uint64_t id){if(!opaque)return MGC_INVALID_ARGUMENT;auto e=effect(opaque);MgcTicket *ticket=nullptr;{std::lock_guard<std::mutex> lock(e->mutex);if(e->busy&&e->request_id==id){e->cancelled=true;ticket=e->ticket;if(ticket)mgc_ticket_retain(ticket);}}if(ticket){auto s=mgr_cancel(e->session,ticket);mgc_ticket_release(ticket);return s;}return MGC_SUCCESS;}
MgcStatus drain(MgmEffect *opaque,uint64_t timeout){return opaque?mgr_drain(effect(opaque)->session,timeout):MGC_INVALID_ARGUMENT;}
}
extern "C" {
MgcStatus mgm_rife_effect_create(MgrPlan *plan,MgmEffect **out,MgmEffectApi *api){if(out)*out=nullptr;if(!plan||!out)return MGC_INVALID_ARGUMENT;MgmEffectApi value{};uint32_t size=0;auto status=copy_desc(api,offsetof(MgmEffectApi,reserved),value,&size);if(status)return status;if(value.required_flags)return MGC_UNSUPPORTED_ABI;if(!zero(value.reserved))return MGC_INVALID_ARGUMENT;auto e=new(std::nothrow)Effect;if(!e)return MGC_OUT_OF_MEMORY;MgcSessionDesc desc{};desc.header=MGC_HEADER_INIT(MgcSessionDesc);desc.max_in_flight=1;status=mgr_session_create(plan,&desc,&e->session);if(status){delete e;return status;}value.query=query;value.prepare=prepare;value.submit=submit;value.cancel=cancel;value.drain=drain;value.estimated_cost_ns=0;write_desc(api,value,size);*out=reinterpret_cast<MgmEffect*>(e);return MGC_SUCCESS;}
MgcStatus mgm_rife_effect_release(MgmEffect *opaque){if(!opaque)return MGC_INVALID_ARGUMENT;auto e=effect(opaque);{std::lock_guard<std::mutex> lock(e->mutex);if(e->busy||e->closing)return MGC_BUSY;e->closing=true;}auto status=mgr_session_close(e->session);if(status){std::lock_guard<std::mutex> lock(e->mutex);e->closing=false;return status;}delete e;return MGC_SUCCESS;}
}
