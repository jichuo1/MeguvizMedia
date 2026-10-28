# MeguvizMedia

当前提供可运行的 C++17 帧/时间线 core、公开 C ABI、单 worker identity effect、可选 RIFE SPI adapter、可安装静态 SDK、显式加载的 Android API/JNI 库及独立图像处理样例。总目标仍为 `bil-rife-ncnn-android`；当前构建产物的适用范围为 CPU/reference，详见 `STATUS.json`。

`src/core` 不依赖 RIFE、Android 或 bil。可选 `MeguvizMedia::rife_integration` 使用 RIFE 公共 API，连接真实 v4.6 FP32 全图 reference。它没有宣称 GPU、AHB、旧 reduced-flow guard 等价或 bil 接管完成。

已运行验证：

- C ABI 边界：先读取 8 字节首部，再复制调用方声明的前缀；旧描述符可省略尾部保留字段，短于最小前缀、未知必需 flag/major 与非零保留字段明确拒绝。真实页边界回归覆盖输入和输出，详见 `artifacts/M1/descriptor-compatibility/summary.json`。
- 纯 C identity consumer：真实资源、一次异步回调、拒绝不转移引用、输出跨 session/domain 外部引用存活。
- 时间线：epoch、旧快照、倍速变化、整数时间换算与溢出；直接运行原 Kotlin oracle，与 C++ normal/delayed 输出逐行对照。
- SDK 安装后在仓库外独立 CMake/C consumer 构建运行。
- 可选 RIFE adapter：模型大小/hash、端点逐字节、alpha0.5 真推理与输出时刻。
- host 组合：从安装 prefix 链接一个 DSO，实际导出与批准名单相等，依赖无第二份 Meguviz/ncnn DSO。
- arm64-v8a 与 armeabi-v7a Android API27 CPU/reference SDK 交叉构建；API-only/JNI AAR 与独立处理 APK 构建。
- 包检查：JVM 类无静态加载器、API-only 无 native、每 ABI 单 native owner、AAR/payload 同字节、ELF 与 APK 16KB 对齐。

Windows 本机复现入口：

```powershell
& 'C:\Users\Administrator\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' tools/run_host.py
& 'C:\Users\Administrator\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' tools/run_rife.py
```

`run_rife.py` 消费 Compute 与 RIFE 的安装 prefix，不自行获取另一份 ncnn。脚本读取合法本地研究模型，模型权重不会进入源码、SDK、AAR、payload 或 APK。

Android 先运行 `tools/build_android.py --help` 并提供实际 NDK/CMake/Ninja 路径，然后在 `android` 运行 wrapper：

```powershell
.\gradlew.bat --offline --no-daemon :media-api:assembleRelease :media-jni:assembleRelease :sample-processing:assembleDebug
```

普通 identity SDK 使用 JNI AAR；按需加载消费者使用 API-only AAR 与同 release AAR 导出的 native payload，不能在同一个应用同时引入两种 native owner。`MediaRuntime` 只有显式加载或 attach 方法，没有静态 `loadLibrary`。

样例是独立图像处理应用，当前未实现完整 MediaCodec 视频播放器。没有设备时只报告已完成的构建/包检查；设备上的实际显示、音画、AHB、GPU 同步和长期资格保持未验证。

ABI 与生命周期权威见 `include/meguviz` 和工作区 `contracts`。来源、许可与当前边界见 `NOTICE.md`、`docs/adr`；所有实际命令、输出与哈希位于 `artifacts`。
