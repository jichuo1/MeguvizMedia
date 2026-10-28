# ADR-0001：Media core、可选算法与宿主边界

状态：已实施，2026-10-28。

Media core 只依赖 Compute 公共 C ABI，拥有帧元数据、内容引用、时钟映射、epoch 与时间线。真实 CPU identity 使用单个有界 worker；回调在物理工作完成后执行，drain 等待回调和提交引用返还。已移交输出继续保持内容与 domain 存活。

`src/core/timeline.cpp` 根据实际 BIL `FrameTimeline.kt` 移植，保留原始 GPLv3 许可。原 Kotlin 文件在验证时直接编译执行，不调用新 C++ 实现；normal 与 delayed/phase-lock 两组各 12 槽的 due、media、输入 ID、alpha 和 mode 与 C++ 输出逐行比较。

RIFE 只在 `integrations/rife` 可选 target 中出现。该适配器使用 RIFE 公共 API，处理双帧提交引用、算法结果、epoch 与输出 PTS，不读取内部 flow/mask 或 ncnn 类型。core 不含 RIFE include、宿主私有符号或 Android 头。

Android binding 使用显式 load/attach。JVM API-only AAR 不拥有 DSO；identity JNI AAR 拥有每 ABI 一份 `libmeguviz_media_identity.so`。同一源 AAR 的最终 DSO 字节被导出为 payload，不再 strip。该 identity 分发是独立 CPU/reference 验证形态，不是目标 RIFE/ncnn 官方组合。

宿主缓存、VisionTool 归档、classloader 约束、AudioTrack 补偿、Surface 接管与回退仍由 bil 集成层负责。
