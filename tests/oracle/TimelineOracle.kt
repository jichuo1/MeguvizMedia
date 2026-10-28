package com.Bilibili_Innocent_Lab.xposedmodule.hook.feature.interpolation
import java.util.Locale

fun main(args: Array<String>) {
    Locale.setDefault(Locale.ROOT)
    val delayed = args.firstOrNull() == "--delayed"
    val timeline = FrameTimeline(60, presentationDelayNs=if(delayed)150_000_000 else 0,
        maxLeadNs=if(delayed)100_000_000 else null, minLeadNs=if(delayed)32_000_000 else 0,
        originalToleranceUs=if(delayed)1000 else 0,phaseLock=delayed,frameWindow=12)
    val lease=timeline.begin(FrameTimeline.Epoch(1,1,1,1,1)).lease
    timeline.updateClock(lease,FrameTimeline.ClockSnapshot(if(delayed)200_000 else 0,0,1.0,false,1))
    for(i in 0L..9L)timeline.offer(lease,FrameTimeline.Frame(i,i*100_000/3))
    val reasons=mapOf(FrameTimeline.Reason.DISABLED to 1,FrameTimeline.Reason.STALE_LEASE to 2,
        FrameTimeline.Reason.NO_CLOCK to 3,FrameTimeline.Reason.PAUSED to 4,FrameTimeline.Reason.STALE_CLOCK to 5,
        FrameTimeline.Reason.INVALID_TIME to 6,FrameTimeline.Reason.IN_FLIGHT to 7,
        FrameTimeline.Reason.AHEAD_OF_WINDOW to 8,FrameTimeline.Reason.NO_PREVIOUS_FRAME to 9,FrameTimeline.Reason.NO_FUTURE_FRAME to 10)
    for(tick in 0L..11L){val now=tick*1_000_000_000/60
        when(val result=timeline.plan(lease,now)){
            is FrameTimeline.Decision.Waiting -> println("W ${reasons[result.reason]} ${result.dueNs?:0} ${result.mediaUs?:0}")
            is FrameTimeline.Decision.Ready -> {val p=result.proposal
                println(String.format(Locale.ROOT,"R %d %d %d %d %.12f %d",p.dueNs,p.mediaUs,p.before.id,p.after.id,p.alpha,if(p.isOriginal)1 else 2))
                check(timeline.commit(p,now))
            }
        }
    }
}
