// SPDX-License-Identifier: GPL-3.0-only
// C++ port of Bilibili_Innocent_Lab FrameTimeline.kt. See NOTICE.md for source identity.
#include <meguviz/timeline.h>
#include "../descriptor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <vector>

namespace {
constexpr int64_t ns_second=1000000000;
bool add(int64_t a,int64_t b,int64_t &v){if((b>0&&a>INT64_MAX-b)||(b<0&&a<INT64_MIN-b))return false;v=a+b;return true;}
bool sub(int64_t a,int64_t b,int64_t &v){if((b<0&&a>INT64_MAX+b)||(b>0&&a<INT64_MIN+b))return false;v=a-b;return true;}
bool mul(int64_t a,int64_t b,int64_t &v){if(a<0||b<0)return false;if(b&&a>INT64_MAX/b)return false;v=a*b;return true;}
bool near(int64_t a,int64_t b,int64_t budget){int64_t d;return budget>=0&&sub(a>=b?a:b,a>=b?b:a,d)&&d<=budget;}
bool fps_ok(uint32_t fps){if(fps==0||fps==15||fps==30||fps==45||fps==50||fps==60||fps==120||fps==144||fps==165||fps==185)return true;for(int source:{24,25,30})for(int k=2;k<=7;++k)if(source*k<=185&&fps==uint32_t(source*k))return true;return false;}
MgcStatus abi(const void *source,size_t size){MgcHeader h{};return mgm_detail::read_header(source,size,h);}
bool rounded(double v,int64_t &out){if(!std::isfinite(v)||v>=double(INT64_MAX)||v<double(INT64_MIN))return false;out=int64_t(std::floor(v+0.5));return true;}
using mgm_detail::copy_desc;
struct Frame {uint64_t id;int64_t pts;};
}
struct MgmPlanner {
    MgmPlannerConfig config{};MgmEpoch epoch{};std::mutex mutex;
    uint64_t generation=0,proposal_sequence=0;bool active=false,phase_locked=false;
    std::optional<MgmClockSnapshot> clock;std::optional<int64_t> anchor,last_observed;
    std::optional<MgmProposal> pending;int64_t pending_tick=0,planned_at=0,next_tick=0,phase_offset=0;
    std::vector<Frame> frames;
    void reset(MgmOfferResult *discarded){if(discarded){discarded->accepted=0;discarded->discarded_count=uint32_t(frames.size());for(size_t i=0;i<frames.size();++i)discarded->discarded_ids[i]=frames[i].id;}frames.clear();pending.reset();clock.reset();anchor.reset();last_observed.reset();next_tick=0;phase_offset=0;phase_locked=false;active=false;}
    bool current(uint64_t g)const{return active&&generation==g;}
    bool observe(int64_t now){if(last_observed&&now<*last_observed)return false;last_observed=now;return true;}
    bool slot_time(int64_t tick,int64_t &out)const{int64_t a,b,c;return tick>=0&&mul(tick/config.target_fps,ns_second,a)&&mul(tick%config.target_fps,ns_second,b)&&add(a,b/config.target_fps,c)&&add(*anchor,c,out);}
    bool first_tick(int64_t elapsed,int64_t &out)const{int64_t a,b,c;return elapsed>=0&&mul(elapsed/ns_second,config.target_fps,a)&&mul(elapsed%ns_second,config.target_fps,b)&&add(b,ns_second-1,c)&&add(a,c/ns_second,out);}
    bool media_at(const MgmClockSnapshot &snapshot,int64_t due,int64_t &out)const{int64_t shown,elapsed,delta,media;bool delay=config.presentation_delay_ns||phase_offset;if(!sub(due,config.presentation_delay_ns,shown)||!sub(shown,snapshot.monotonic_ns,elapsed))return false;if((!delay&&elapsed<0)||(delay&&elapsed<-(config.max_clock_age_ns+config.presentation_delay_ns)))return false;if(!rounded(double(elapsed)/1000.0*snapshot.speed,delta)||!add(snapshot.media_us,delta,media)||!sub(media,phase_offset,out))return false;return out>=0;}
    MgcStatus valid(const MgmProposal *p,int64_t now){if(!pending||!current(p->lease_generation)||pending->proposal_id!=p->proposal_id)return MGC_CANCELLED;if(!observe(now)||now<planned_at||now>pending->due_ns||!clock||clock->paused)return MGC_CANCELLED;int64_t age;if(!sub(now,clock->monotonic_ns,age)||age<0||age>config.max_clock_age_ns)return MGC_CANCELLED;return MGC_SUCCESS;}
};
extern "C" {
MgcStatus mgm_planner_create(const MgmPlannerConfig *c,MgmPlanner **out){if(out)*out=nullptr;if(!out)return MGC_INVALID_ARGUMENT;MgmPlannerConfig copied{};auto s=copy_desc(c,offsetof(MgmPlannerConfig,required_flags)+sizeof(uint32_t),copied);if(s)return s;c=&copied;if(c->required_flags)return MGC_UNSUPPORTED_ABI;int64_t period=c->target_fps?(ns_second+c->target_fps-1)/c->target_fps:1;if(!fps_ok(c->target_fps)||c->frame_window<4||c->frame_window>12||c->max_clock_age_ns<=0||c->max_clock_age_ns>ns_second||c->presentation_delay_ns<0||c->presentation_delay_ns>250000000||c->max_lead_ns<0||c->max_lead_ns>250000000||c->min_lead_ns<0||c->min_lead_ns>(c->max_lead_ns?c->max_lead_ns:period)||c->original_tolerance_us<0||c->max_clock_uncertainty_us<0||c->max_clock_uncertainty_us>8000||c->phase_lock>1)return MGC_INVALID_ARGUMENT;auto p=new(std::nothrow) MgmPlanner;if(!p)return MGC_OUT_OF_MEMORY;try{p->frames.reserve(c->frame_window);}catch(const std::bad_alloc&){delete p;return MGC_OUT_OF_MEMORY;}p->config=*c;*out=p;return MGC_SUCCESS;}
void mgm_planner_release(MgmPlanner *p){delete p;}
MgcStatus mgm_planner_begin(MgmPlanner *p,const MgmEpoch *epoch,uint64_t *generation,MgmOfferResult *discarded){if(!p||!epoch||!generation)return MGC_INVALID_ARGUMENT;auto status=abi(discarded,sizeof(*discarded));if(status)return status;std::lock_guard<std::mutex> lock(p->mutex);p->reset(discarded);p->epoch=*epoch;if(p->generation==UINT64_MAX)return MGC_INTERNAL_ERROR;*generation=++p->generation;p->active=true;return MGC_SUCCESS;}
MgcStatus mgm_planner_clock(MgmPlanner *p,uint64_t generation,const MgmClockSnapshot *s,uint32_t *state,MgmOfferResult *discarded){if(!p||!state)return MGC_INVALID_ARGUMENT;auto output_status=abi(discarded,sizeof(*discarded));if(output_status)return output_status;MgmClockSnapshot copied{};if(s){auto status=copy_desc(s,offsetof(MgmClockSnapshot,required_flags)+sizeof(uint32_t),copied);if(status)return status;if(copied.required_flags)return MGC_UNSUPPORTED_ABI;s=&copied;}std::lock_guard<std::mutex> lock(p->mutex);discarded->discarded_count=0;if(!p->current(generation))return MGC_CANCELLED;if(!s){*state=MGM_CLOCK_RESTART_REQUIRED;p->reset(discarded);return MGC_SUCCESS;}
    if(s->media_us<0||s->uncertainty_us<0||s->uncertainty_us>p->config.max_clock_uncertainty_us||!std::isfinite(s->speed)||s->speed<0||(!s->paused&&s->speed==0)||s->discontinuity!=p->epoch.discontinuity){*state=MGM_CLOCK_RESTART_REQUIRED;p->reset(discarded);return MGC_SUCCESS;}
    if(p->clock){auto &prev=*p->clock;if(s->monotonic_ns<prev.monotonic_ns){*state=MGM_CLOCK_STALE_SNAPSHOT;return MGC_SUCCESS;}int64_t budget=prev.uncertainty_us+s->uncertainty_us;if(s->speed!=prev.speed||s->paused!=prev.paused||(s->media_us<prev.media_us&&!near(s->media_us,prev.media_us,budget))||(s->monotonic_ns==prev.monotonic_ns&&!near(s->media_us,prev.media_us,budget))){*state=MGM_CLOCK_RESTART_REQUIRED;p->reset(discarded);return MGC_SUCCESS;}}
    if(p->pending){int64_t updated;if(!p->media_at(*s,p->pending->due_ns,updated)||!near(updated,p->pending->media_us,p->pending->uncertainty_us+s->uncertainty_us))p->pending.reset();}
    p->clock=*s;if(!p->anchor){p->anchor=s->monotonic_ns;p->next_tick=0;}*state=s->paused?MGM_CLOCK_PAUSED_STATE:MGM_CLOCK_READY;return MGC_SUCCESS;
}
MgcStatus mgm_planner_offer(MgmPlanner *p,uint64_t generation,uint64_t id,int64_t pts,MgmOfferResult *out){if(!p||!out)return MGC_INVALID_ARGUMENT;auto s=abi(&out->header,sizeof(*out));if(s)return s;std::lock_guard<std::mutex> lock(p->mutex);out->accepted=0;out->discarded_count=0;
    for(auto &f:p->frames)if(f.id==id)return MGC_SUCCESS;
    if(!p->current(generation)||!p->config.target_fps||pts<0){out->discarded_count=1;out->discarded_ids[0]=id;return MGC_SUCCESS;}
    for(auto &f:p->frames)if(f.pts==pts){out->discarded_count=1;out->discarded_ids[0]=f.id;f={id,pts};out->accepted=1;if(p->pending&&(p->pending->before_id==out->discarded_ids[0]||p->pending->after_id==out->discarded_ids[0]))p->pending.reset();return MGC_SUCCESS;}
    if(!p->frames.empty()&&pts<p->frames.back().pts){out->discarded_count=1;out->discarded_ids[0]=id;return MGC_SUCCESS;}
    try{if(p->frames.size()==p->config.frame_window){out->discarded_count=1;out->discarded_ids[0]=p->frames.front().id;p->frames.erase(p->frames.begin());}p->frames.push_back({id,pts});}catch(const std::bad_alloc&){return MGC_OUT_OF_MEMORY;}
    if(p->pending&&out->discarded_count&&(p->pending->before_id==out->discarded_ids[0]||p->pending->after_id==out->discarded_ids[0]))p->pending.reset();out->accepted=1;return MGC_SUCCESS;
}
MgcStatus mgm_planner_plan(MgmPlanner *p,uint64_t generation,int64_t now,MgmProposal *out){if(!p||!out)return MGC_INVALID_ARGUMENT;auto s=abi(&out->header,sizeof(*out));if(s)return s;std::lock_guard<std::mutex> lock(p->mutex);MgcHeader h=out->header;*out={};out->header=h;out->lease_generation=generation;auto wait=[&](uint32_t reason){out->waiting_reason=reason;return MGC_SUCCESS;};if(!p->current(generation))return wait(MGM_WAIT_STALE_LEASE);if(!p->config.target_fps)return wait(MGM_WAIT_DISABLED);if(!p->clock)return wait(MGM_WAIT_NO_CLOCK);auto clock=*p->clock;if(clock.paused)return wait(MGM_WAIT_PAUSED);if(!p->observe(now))return wait(MGM_WAIT_INVALID_TIME);int64_t age;if(!sub(now,clock.monotonic_ns,age)||age<0)return wait(MGM_WAIT_INVALID_TIME);if(age>p->config.max_clock_age_ns)return wait(MGM_WAIT_STALE_CLOCK);
    if(p->pending){if(now<=p->pending->due_ns){out->due_ns=p->pending->due_ns;out->media_us=p->pending->media_us;return wait(MGM_WAIT_IN_FLIGHT);}p->pending.reset();}
    int64_t elapsed,tick;if(!p->anchor||!sub(now,*p->anchor,elapsed)||!p->first_tick(elapsed,tick))return wait(MGM_WAIT_INVALID_TIME);p->next_tick=std::max(p->next_tick,tick);auto max_lead=p->config.max_lead_ns?p->config.max_lead_ns:(ns_second+p->config.target_fps-1)/p->config.target_fps;
    for(int scan=0;scan<8;++scan){int64_t due,ahead,media;if(!p->slot_time(p->next_tick,due)||!sub(due,now,ahead))return wait(MGM_WAIT_INVALID_TIME);out->due_ns=due;if(ahead>max_lead)return wait(MGM_WAIT_AHEAD_OF_WINDOW);
        if(p->config.phase_lock&&!p->phase_locked){int64_t base;if(!p->media_at(clock,due,base))return wait(MGM_WAIT_INVALID_TIME);const Frame *reference=nullptr;for(auto &f:p->frames)if(f.pts<=base)reference=&f;if(!reference){out->media_us=base;return wait(MGM_WAIT_NO_PREVIOUS);}p->phase_offset=base-reference->pts;p->phase_locked=true;}
        if(!p->media_at(clock,due,media))return wait(MGM_WAIT_INVALID_TIME);
        const Frame *nearest=nullptr;int64_t nearest_distance=INT64_MAX;for(auto &f:p->frames){int64_t distance;if(sub(std::max(f.pts,media),std::min(f.pts,media),distance)&&distance<nearest_distance){nearest=&f;nearest_distance=distance;}}
        if(p->config.phase_lock&&nearest&&nearest->pts!=media){double period=1000000.0*clock.speed/p->config.target_fps;int64_t correction,lower;if(nearest_distance<period/2&&sub(media,nearest->pts,correction)&&add(p->phase_offset,correction,correction)&&rounded(period,lower)&&correction>=-lower&&correction<=100000){p->phase_offset=correction;media=nearest->pts;nearest_distance=0;}}
        out->media_us=media;const Frame *exact=nearest&&nearest_distance<=p->config.original_tolerance_us?nearest:nullptr;const Frame *before=exact,*after=exact;if(!exact){for(auto &f:p->frames){if(f.pts<media)before=&f;if(f.pts>media&&!after)after=&f;}}if(!before)return wait(MGM_WAIT_NO_PREVIOUS);if(!after)return wait(MGM_WAIT_NO_FUTURE);
        const Frame *selected=exact;double period=std::ceil(1000000.0*clock.speed/p->config.target_fps);if(!selected&&double(after->pts-before->pts)<=period)selected=media-before->pts<=after->pts-media?before:after;
        if(!selected&&ahead<p->config.min_lead_ns){if(p->next_tick==INT64_MAX)return wait(MGM_WAIT_INVALID_TIME);++p->next_tick;continue;}
        out->proposal_id=++p->proposal_sequence;out->before_id=selected?selected->id:before->id;out->after_id=selected?selected->id:after->id;out->alpha=selected?0.0:double(media-before->pts)/double(after->pts-before->pts);out->uncertainty_us=clock.uncertainty_us;out->mode=selected?MGM_FRAME_ORIGINAL:MGM_FRAME_INFERRED;p->pending=*out;p->pending_tick=p->next_tick;p->planned_at=now;return MGC_SUCCESS;
    }return wait(MGM_WAIT_AHEAD_OF_WINDOW);
}
MgcStatus mgm_planner_valid(MgmPlanner *p,const MgmProposal *proposal,int64_t now){if(!p)return MGC_INVALID_ARGUMENT;MgmProposal value{};auto s=copy_desc(proposal,sizeof(*proposal),value);if(s)return s;std::lock_guard<std::mutex> lock(p->mutex);return p->valid(&value,now);}
MgcStatus mgm_planner_commit(MgmPlanner *p,const MgmProposal *proposal,int64_t now){if(!p)return MGC_INVALID_ARGUMENT;MgmProposal value{};auto s=copy_desc(proposal,sizeof(*proposal),value);if(s)return s;proposal=&value;std::lock_guard<std::mutex> lock(p->mutex);s=p->valid(proposal,now);if(s){if(p->pending&&p->pending->proposal_id==proposal->proposal_id)p->pending.reset();return s;}if(p->pending_tick==INT64_MAX)return MGC_INVALID_ARGUMENT;p->next_tick=p->pending_tick+1;p->pending.reset();return MGC_SUCCESS;}
MgcStatus mgm_ticks_to_ns(int64_t ticks,MgmRational tb,int64_t *out){if(!out||tb.numerator<=0||tb.denominator<=0||ticks<0)return MGC_INVALID_ARGUMENT;int64_t seconds,remainder,multiplier,fraction;if(!mul(tb.numerator,ns_second,multiplier)||!mul(ticks/tb.denominator,multiplier,seconds)||!mul(ticks%tb.denominator,multiplier,remainder))return MGC_INVALID_ARGUMENT;fraction=remainder/tb.denominator;if(!add(seconds,fraction,*out))return MGC_INVALID_ARGUMENT;return MGC_SUCCESS;}
}
