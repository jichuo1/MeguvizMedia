#ifndef MEGUVIZ_TIMELINE_H
#define MEGUVIZ_TIMELINE_H
#include "media.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MgmPlanner MgmPlanner;
typedef struct MgmEpoch { uint64_t media,decoder,surface,format,discontinuity; } MgmEpoch;
enum { MGM_WAIT_DISABLED=1,MGM_WAIT_STALE_LEASE=2,MGM_WAIT_NO_CLOCK=3,MGM_WAIT_PAUSED=4,MGM_WAIT_STALE_CLOCK=5,MGM_WAIT_INVALID_TIME=6,MGM_WAIT_IN_FLIGHT=7,MGM_WAIT_AHEAD_OF_WINDOW=8,MGM_WAIT_NO_PREVIOUS=9,MGM_WAIT_NO_FUTURE=10 };
enum { MGM_CLOCK_READY=1,MGM_CLOCK_PAUSED_STATE=2,MGM_CLOCK_STALE_SNAPSHOT=3,MGM_CLOCK_RESTART_REQUIRED=4 };
typedef struct MgmPlannerConfig {
    MgcHeader header;
    uint32_t target_fps;
    uint32_t frame_window;
    int64_t max_clock_age_ns;
    int64_t presentation_delay_ns;
    int64_t max_lead_ns;
    int64_t min_lead_ns;
    int64_t original_tolerance_us;
    int64_t max_clock_uncertainty_us;
    uint32_t phase_lock;
    uint32_t required_flags;
} MgmPlannerConfig;
typedef struct MgmClockSnapshot {
    MgcHeader header;
    int64_t media_us;
    int64_t monotonic_ns;
    double speed;
    uint64_t discontinuity;
    int64_t uncertainty_us;
    uint32_t paused;
    uint32_t required_flags;
} MgmClockSnapshot;
typedef struct MgmOfferResult {
    MgcHeader header;
    uint32_t accepted;
    uint32_t discarded_count;
    uint64_t discarded_ids[12];
} MgmOfferResult;
typedef struct MgmProposal {
    MgcHeader header;
    uint64_t proposal_id;
    uint64_t lease_generation;
    int64_t due_ns;
    int64_t media_us;
    uint64_t before_id;
    uint64_t after_id;
    double alpha;
    int64_t uncertainty_us;
    uint32_t mode;
    uint32_t waiting_reason;
} MgmProposal;
/* Config/snapshot require fields through required_flags; proposal and output
 * records require their full current prefix. Future trailing fields are ignored. */
MGM_API MgcStatus mgm_planner_create(const MgmPlannerConfig *config,MgmPlanner **planner);
MGM_API void mgm_planner_release(MgmPlanner *planner);
MGM_API MgcStatus mgm_planner_begin(MgmPlanner *planner,const MgmEpoch *epoch,uint64_t *lease_generation,MgmOfferResult *discarded);
MGM_API MgcStatus mgm_planner_clock(MgmPlanner *planner,uint64_t lease_generation,const MgmClockSnapshot *snapshot,uint32_t *clock_state,MgmOfferResult *discarded);
MGM_API MgcStatus mgm_planner_offer(MgmPlanner *planner,uint64_t lease_generation,uint64_t frame_id,int64_t pts_us,MgmOfferResult *result);
MGM_API MgcStatus mgm_planner_plan(MgmPlanner *planner,uint64_t lease_generation,int64_t now_ns,MgmProposal *proposal);
MGM_API MgcStatus mgm_planner_valid(MgmPlanner *planner,const MgmProposal *proposal,int64_t now_ns);
MGM_API MgcStatus mgm_planner_commit(MgmPlanner *planner,const MgmProposal *proposal,int64_t now_ns);
/* Rational conversion uses checked integer arithmetic; overflow is rejected. */
MGM_API MgcStatus mgm_ticks_to_ns(int64_t ticks,MgmRational timebase,int64_t *nanoseconds);
#ifdef __cplusplus
}
#endif
#endif
