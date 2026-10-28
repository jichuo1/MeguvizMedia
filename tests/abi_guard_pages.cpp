#include <meguviz/media.h>
#include <meguviz/timeline.h>
#include "descriptor_pages.h"
#include <atomic>
static std::atomic<int> callbacks{0};
void completed(void*,uint64_t id,MgcStatus status,MgmFrame *output){CHECK(id==73&&status==MGC_SUCCESS&&output);mgm_frame_release(output);++callbacks;}
void unexpected(void*,uint64_t,MgcStatus,MgmFrame*){CHECK(false);}
template<class T>T init(){T value{};value.header=MGC_HEADER_INIT(T);return value;}
int main(){
    DescriptorPages short_value;MgmFrame *frame=nullptr;MgmTimeline *timeline=nullptr;MgmPlanner *planner=nullptr;
    CHECK(mgm_frame_create(short_value.as<MgmFrameDesc>(),&frame)==MGC_INVALID_ARGUMENT&&!frame);
    CHECK(mgm_timeline_create(short_value.as<MgmClockMapping>(),&timeline)==MGC_INVALID_ARGUMENT&&!timeline);
    CHECK(mgm_planner_create(short_value.as<MgmPlannerConfig>(),&planner)==MGC_INVALID_ARGUMENT&&!planner);
    auto rd=init<MgcRuntimeDesc>();rd.provider=MGC_PROVIDER_CPU_REFERENCE;rd.memory_budget_bytes=4096;MgcRuntime *runtime=nullptr;CHECK(mgc_runtime_create(&rd,&runtime)==MGC_SUCCESS);
    auto dd=init<MgcDomainDesc>();MgcDomain *domain=nullptr;CHECK(mgc_domain_acquire(runtime,&dd,&domain)==MGC_SUCCESS);
    auto resource_desc=init<MgcResourceDesc>();resource_desc.capacity_bytes=12;resource_desc.width=2;resource_desc.height=2;resource_desc.row_stride_bytes=6;resource_desc.format=MGC_FORMAT_RGB8;MgcResource *resource=nullptr;CHECK(mgc_resource_create(domain,&resource_desc,&resource)==MGC_SUCCESS);
    auto ld=init<MgcLeaseDesc>();ld.content_id=1;ld.byte_length=12;ld.access=MGC_ACCESS_READ;MgcLease *lease=nullptr;CHECK(mgc_lease_create(resource,&ld,&lease)==MGC_SUCCESS);
    auto fd=init<MgmFrameDesc>();fd.content=lease;fd.timebase={1,1000};fd.epoch=1;fd.frame_kind=MGM_FRAME_ORIGINAL;
    DescriptorPages frame_prefix(offsetof(MgmFrameDesc,reserved));frame_prefix.store(fd);CHECK(mgm_frame_create(frame_prefix.as<MgmFrameDesc>(),&frame)==MGC_SUCCESS&&frame);
    CHECK(mgm_frame_describe(frame,short_value.as<MgmFrameDesc>())==MGC_SUCCESS);CHECK(short_value.read<MgcHeader>().struct_size==sizeof(MgcHeader));
    DescriptorPages frame_short(offsetof(MgmFrameDesc,reserved)-1);frame_short.store(fd);MgmFrame *rejected=nullptr;CHECK(mgm_frame_create(frame_short.as<MgmFrameDesc>(),&rejected)==MGC_INVALID_ARGUMENT&&!rejected);
    fd.required_flags=1;frame_prefix.store(fd);CHECK(mgm_frame_create(frame_prefix.as<MgmFrameDesc>(),&rejected)==MGC_UNSUPPORTED_ABI&&!rejected);fd.required_flags=0;
    fd.header.abi_major=1;frame_prefix.store(fd);CHECK(mgm_frame_create(frame_prefix.as<MgmFrameDesc>(),&rejected)==MGC_UNSUPPORTED_ABI&&!rejected);fd.header.abi_major=0;
    fd.reserved[2]=1;DescriptorPages frame_full(sizeof(fd));frame_full.store(fd);CHECK(mgm_frame_create(frame_full.as<MgmFrameDesc>(),&rejected)==MGC_INVALID_ARGUMENT&&!rejected);fd.reserved[2]=0;
    DescriptorPages future_frame(sizeof(fd)+16);future_frame.store(fd);std::memset(static_cast<char*>(future_frame.value)+sizeof(fd),0xA5,16);CHECK(mgm_frame_create(future_frame.as<MgmFrameDesc>(),&rejected)==MGC_SUCCESS&&rejected);mgm_frame_release(rejected);
    auto mapping=init<MgmClockMapping>();mapping.valid=1;mapping.epoch=1;mapping.timebase={1,1000};DescriptorPages mapping_prefix(offsetof(MgmClockMapping,reserved));mapping_prefix.store(mapping);CHECK(mgm_timeline_create(mapping_prefix.as<MgmClockMapping>(),&timeline)==MGC_SUCCESS);
    CHECK(mgm_timeline_reset(timeline,short_value.as<MgmClockMapping>())==MGC_INVALID_ARGUMENT);mapping.epoch=2;mapping_prefix.store(mapping);CHECK(mgm_timeline_reset(timeline,mapping_prefix.as<MgmClockMapping>())==MGC_SUCCESS);mapping.reserved[0]=1;CHECK(mgm_timeline_reset(timeline,&mapping)==MGC_INVALID_ARGUMENT);mapping.reserved[0]=0;
    MgmEffect *effect=nullptr;CHECK(mgm_identity_effect_create(domain,&effect,short_value.as<MgmEffectApi>())==MGC_INVALID_ARGUMENT&&!effect);
    auto api=init<MgmEffectApi>();DescriptorPages api_prefix(offsetof(MgmEffectApi,reserved));api_prefix.store(api);CHECK(mgm_identity_effect_create(domain,&effect,api_prefix.as<MgmEffectApi>())==MGC_SUCCESS&&effect);api=api_prefix.read<MgmEffectApi>();
    CHECK(api.prepare(effect,short_value.as<MgmFrameDesc>())==MGC_INVALID_ARGUMENT);frame_prefix.store(fd);CHECK(api.prepare(effect,frame_prefix.as<MgmFrameDesc>())==MGC_SUCCESS);
    CHECK(api.submit(effect,short_value.as<MgmEffectRequest>(),unexpected,nullptr,nullptr,nullptr)==MGC_INVALID_ARGUMENT);
    CHECK(api.query(effect,short_value.as<MgcCapability>())==MGC_INVALID_ARGUMENT);DescriptorPages capability_prefix(offsetof(MgcCapability,reserved));CHECK(api.query(effect,capability_prefix.as<MgcCapability>())==MGC_SUCCESS);CHECK(capability_prefix.read<MgcCapability>().enabled==1);
    auto capability=init<MgcCapability>();capability.header.abi_major=1;capability_prefix.store(capability);CHECK(api.query(effect,capability_prefix.as<MgcCapability>())==MGC_UNSUPPORTED_ABI);
    auto request=init<MgmEffectRequest>();request.request_id=73;request.epoch=1;request.left=frame;DescriptorPages request_prefix(offsetof(MgmEffectRequest,reserved));request_prefix.store(request);mgm_frame_retain(frame);CHECK(api.submit(effect,request_prefix.as<MgmEffectRequest>(),completed,nullptr,nullptr,nullptr)==MGC_ACCEPTED);CHECK(api.drain(effect,2000000000)==MGC_SUCCESS&&callbacks==1);
    request.required_flags=1;request_prefix.store(request);CHECK(api.submit(effect,request_prefix.as<MgmEffectRequest>(),unexpected,nullptr,nullptr,nullptr)==MGC_UNSUPPORTED_ABI);request.required_flags=0;request.reserved[0]=1;CHECK(api.submit(effect,&request,unexpected,nullptr,nullptr,nullptr)==MGC_INVALID_ARGUMENT);
    auto invalid_api=init<MgmEffectApi>();invalid_api.required_flags=1;MgmEffect *rejected_effect=nullptr;CHECK(mgm_identity_effect_create(domain,&rejected_effect,&invalid_api)==MGC_UNSUPPORTED_ABI&&!rejected_effect);invalid_api.required_flags=0;invalid_api.reserved[0]=1;CHECK(mgm_identity_effect_create(domain,&rejected_effect,&invalid_api)==MGC_INVALID_ARGUMENT&&!rejected_effect);
    auto config=init<MgmPlannerConfig>();config.target_fps=60;config.frame_window=4;config.max_clock_age_ns=250000000;config.max_clock_uncertainty_us=2000;DescriptorPages config_prefix(offsetof(MgmPlannerConfig,required_flags)+sizeof(uint32_t));config_prefix.store(config);CHECK(mgm_planner_create(config_prefix.as<MgmPlannerConfig>(),&planner)==MGC_SUCCESS);
    auto discarded=init<MgmOfferResult>();MgmEpoch epoch{1,1,1,1,1};uint64_t generation=0;CHECK(mgm_planner_begin(planner,&epoch,&generation,short_value.as<MgmOfferResult>())==MGC_INVALID_ARGUMENT);discarded.header.abi_major=1;CHECK(mgm_planner_begin(planner,&epoch,&generation,&discarded)==MGC_UNSUPPORTED_ABI);discarded.header.abi_major=0;CHECK(mgm_planner_begin(planner,&epoch,&generation,&discarded)==MGC_SUCCESS);
    uint32_t state=0;CHECK(mgm_planner_clock(planner,generation,short_value.as<MgmClockSnapshot>(),&state,&discarded)==MGC_INVALID_ARGUMENT);
    auto snapshot=init<MgmClockSnapshot>();snapshot.speed=1;snapshot.discontinuity=1;DescriptorPages clock_prefix(offsetof(MgmClockSnapshot,required_flags)+sizeof(uint32_t));clock_prefix.store(snapshot);CHECK(mgm_planner_clock(planner,generation,clock_prefix.as<MgmClockSnapshot>(),&state,&discarded)==MGC_SUCCESS&&state==MGM_CLOCK_READY);
    snapshot.required_flags=1;clock_prefix.store(snapshot);CHECK(mgm_planner_clock(planner,generation,clock_prefix.as<MgmClockSnapshot>(),&state,&discarded)==MGC_UNSUPPORTED_ABI);
    CHECK(mgm_planner_valid(planner,short_value.as<MgmProposal>(),0)==MGC_INVALID_ARGUMENT);CHECK(mgm_planner_commit(planner,short_value.as<MgmProposal>(),0)==MGC_INVALID_ARGUMENT);CHECK(mgm_planner_offer(planner,generation,1,0,short_value.as<MgmOfferResult>())==MGC_INVALID_ARGUMENT);CHECK(mgm_planner_plan(planner,generation,0,short_value.as<MgmProposal>())==MGC_INVALID_ARGUMENT);
    auto proposal=init<MgmProposal>();proposal.header.abi_major=1;CHECK(mgm_planner_valid(planner,&proposal,0)==MGC_UNSUPPORTED_ABI);CHECK(mgm_planner_commit(planner,&proposal,0)==MGC_UNSUPPORTED_ABI);
    mgm_planner_release(planner);CHECK(mgm_identity_effect_release(effect)==MGC_SUCCESS);mgm_timeline_release(timeline);mgm_frame_release(frame);mgc_lease_release(lease);mgc_resource_release(resource);mgc_domain_release(domain);CHECK(mgc_runtime_close(runtime)==MGC_SUCCESS);mgc_runtime_release(runtime);
    std::cout<<"{\"result\":\"PASS\",\"scope\":\"real_page_boundary_public_descriptors\",\"header_only_rejections\":true,\"minimum_prefix_success\":true,\"future_tail_ignored\":true,\"flags_and_reserved_rejected\":true}"<<std::endl;
}
