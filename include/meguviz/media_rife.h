#ifndef MEGUVIZ_MEDIA_RIFE_H
#define MEGUVIZ_MEDIA_RIFE_H
#include "media.h"
#include <meguviz/rife.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Optional adapter only; Media core never includes the RIFE header. */
MGM_API MgcStatus mgm_rife_effect_create(MgrPlan *plan,MgmEffect **effect,MgmEffectApi *api);
MGM_API MgcStatus mgm_rife_effect_release(MgmEffect *effect);
#ifdef __cplusplus
}
#endif
#endif
