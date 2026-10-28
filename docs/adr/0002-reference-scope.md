# ADR-0002：当前证据范围与未资格路径

状态：已实施，2026-10-28。

总目标保持 `bil-rife-ncnn-android`。当前可运行证据包括 CPU identity、时间线、安装消费、真实 v4.6 FP32 全图算法适配器及一个 host DSO 的静态链接闭包。FP32/full-scale/guard0 是独立 reference profile，不替代 bil 的 FP16/reduced-flow/完整 guard profile。

两个 Android ABI 已实际交叉构建并组装成独立样例。ELF LOAD 对齐、导出、动态依赖、AAR native owner、API 字节和 APK zip 对齐已检查；这些检查不等于设备装载、16KB 运行、视频显示或 A/V 资格。

当前没有已资格的 AHB_HOST_WAIT、AHB_GPU_SYNC、decoder AHB、HDR、SurfaceControl 或 MediaCodec 播放链。依赖 C2/C3/R2 尚未完成时，不将接口、构建成功或 CPU 输出称为这些路径完成。

时间线保留原 150ms delay/min-lead/phase-lock 配置能力；默认生产策略未在 bil 中切换。未采集端到端 GPU、实际呈现、功率和声画 raw 证据，相关性能和质量 gate 保持 INCONCLUSIVE。
