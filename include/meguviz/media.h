#ifndef MEGUVIZ_MEDIA_H
#define MEGUVIZ_MEDIA_H
#include <meguviz/compute.h>

#ifdef __cplusplus
extern "C" {
#endif
#define MGM_API MGC_API
#define MGM_SPI_MAJOR 0u
#define MGM_SPI_MINOR 1u
typedef struct MgmFrame MgmFrame;
typedef struct MgmEffect MgmEffect;
typedef struct MgmTimeline MgmTimeline;
enum { MGM_FRAME_ORIGINAL = 1, MGM_FRAME_INFERRED = 2, MGM_FRAME_ENDPOINT = 3, MGM_FRAME_BYPASS = 4 };
enum { MGM_TRANSFER_SDR = 1, MGM_TRANSFER_PQ = 2, MGM_TRANSFER_HLG = 3 };
enum { MGM_RANGE_FULL = 1, MGM_RANGE_LIMITED = 2 };
enum { MGM_CLOCK_MEDIA_PTS = 1, MGM_CLOCK_CPU_MONOTONIC = 2, MGM_CLOCK_GPU = 3, MGM_CLOCK_DISPLAY = 4 };

typedef struct MgmRational { int32_t numerator; int32_t denominator; } MgmRational;
typedef struct MgmColor {
    MgcHeader header;
    uint32_t primaries;
    uint32_t transfer;
    uint32_t range;
    uint32_t matrix;
    uint32_t chroma_siting;
    uint32_t bit_depth;
    uint32_t premultiplied;
    uint32_t reserved;
} MgmColor;
typedef struct MgmFrameDesc {
    MgcHeader header;
    MgcLease *content;
    int64_t pts_ticks;
    MgmRational timebase;
    uint64_t epoch;
    MgmColor color;
    uint32_t rotation_degrees;
    uint32_t crop_left;
    uint32_t crop_top;
    uint32_t crop_width;
    uint32_t crop_height;
    uint32_t frame_kind;
    uint32_t required_flags;
    uint32_t reserved[3];
} MgmFrameDesc;
typedef struct MgmClockMapping {
    MgcHeader header;
    uint64_t epoch;
    int64_t media_origin_ticks;
    MgmRational timebase;
    int64_t cpu_origin_ns;
    uint64_t error_bound_ns;
    uint32_t valid;
    uint32_t reserved[3];
} MgmClockMapping;
typedef struct MgmEffectRequest {
    MgcHeader header;
    uint64_t request_id;
    uint64_t epoch;
    MgmFrame *left;
    MgmFrame *right;
    double alpha;
    int64_t deadline_cpu_ns;
    uint32_t required_flags;
    uint32_t reserved[3];
} MgmEffectRequest;
typedef void (*MgmEffectCompletionFn)(void *context, uint64_t request_id, MgcStatus status, MgmFrame *owned_output);
typedef struct MgmEffectApi {
    MgcHeader header;
    MgcStatus (*query)(MgmEffect *effect, MgcCapability *capability);
    MgcStatus (*prepare)(MgmEffect *effect, const MgmFrameDesc *format);
    MgcStatus (*submit)(MgmEffect *effect, const MgmEffectRequest *request,
                        MgmEffectCompletionFn completion, void *context,
                        MgcContextFn retain_context, MgcContextFn release_context);
    MgcStatus (*cancel)(MgmEffect *effect, uint64_t request_id);
    MgcStatus (*drain)(MgmEffect *effect, uint64_t timeout_ns);
    uint64_t estimated_cost_ns;
    uint32_t required_flags;
    uint32_t reserved[3];
} MgmEffectApi;
/* Descriptor inputs require the prefix through the last non-reserved field;
 * omitted trailing reserved fields are zero. Unknown required_flags or major
 * versions are rejected before any body field is used. Outputs write at most
 * struct_size bytes; frame_describe also supports a header-only old caller.
 * create borrows desc and retains content; clone adds a reference, no pixels. */
MGM_API MgcStatus mgm_frame_create(const MgmFrameDesc *desc, MgmFrame **frame);
MGM_API void mgm_frame_retain(MgmFrame *frame);
MGM_API void mgm_frame_release(MgmFrame *frame);
MGM_API MgcStatus mgm_frame_describe(MgmFrame *frame, MgmFrameDesc *desc);
MGM_API MgcStatus mgm_timeline_create(const MgmClockMapping *mapping, MgmTimeline **timeline);
MGM_API void mgm_timeline_release(MgmTimeline *timeline);
/* epoch is strictly monotonic; old epoch completion may retire but not present. */
MGM_API MgcStatus mgm_timeline_reset(MgmTimeline *timeline, const MgmClockMapping *mapping);
MGM_API MgcStatus mgm_timeline_may_present(MgmTimeline *timeline, MgmFrame *frame);
/* Identity transports a frame without copying pixels or claiming inference.
 * submit transfers one caller-provided reference for each non-null input on
 * ACCEPTED. Rejection transfers none. Callback owns one output reference.
 * release returns BUSY on its worker or while accepted work remains. */
MGM_API MgcStatus mgm_identity_effect_create(MgcDomain *domain, MgmEffect **effect, MgmEffectApi *api);
MGM_API MgcStatus mgm_identity_effect_release(MgmEffect *effect);

#ifdef __cplusplus
}
#endif
#endif
