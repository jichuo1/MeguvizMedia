# 来源与资格范围

`src/core/timeline.cpp` 根据 Bilibili_Innocent_Lab 的 `FrameTimeline.kt` 移植。
原仓库使用 GPLv3；本项目保留同一许可文件。源身份和实际源码 SHA256 记录在 `artifacts/M0/source-inventory.json`。
其余 Media 新代码与上述模块共同按 GPLv3 交付。公开头文件不包含 ncnn、Android 或宿主私有类型。

identity effect 只保留内容 lease 与元数据，不生成 RIFE 推理结果，也不声称 GPU 加速。
HOST_WAIT、SYNC_FD、解码器 AHB、HDR、SurfaceControl 与实际 A/V 资格必须有单独设备证据；缺证据不加入支持清单。
