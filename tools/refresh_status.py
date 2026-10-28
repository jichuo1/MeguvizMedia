import hashlib
import json
from pathlib import Path

root=Path(__file__).resolve().parents[1]
last=json.loads((root/"artifacts/M1/latest-rife.json").read_text())
rife=Path(last["summary"])
package=root/"artifacts/M1/android-package/manifest.json"
core=root/"artifacts/M1/install/lib/meguviz_media_core.lib"
compatibility=root/"artifacts/M1/descriptor-compatibility/summary.json"
selected=[root/"artifacts/M0/source-inventory.json",root/"artifacts/M1/commands.jsonl",root/"artifacts/M1/ctest.stdout.txt",root/"artifacts/M1/cpp_normal.stdout.txt",root/"artifacts/M1/kotlin_normal.stdout.txt",root/"artifacts/M1/cpp_delayed.stdout.txt",root/"artifacts/M1/kotlin_delayed.stdout.txt",core,rife,package,root/"artifacts/M1/android-gradle.stdout.txt",root/"artifacts/M1/android-commands.jsonl",root/"artifacts/M1/android-gradle-command.json",root/"artifacts/M1/android-arm64-v8a/manifest.json",root/"artifacts/M1/android-armeabi-v7a/manifest.json",root/"artifacts/M1/ctest-details.stdout.txt",compatibility]
for p in selected:
    if not p.is_file():raise RuntimeError("missing evidence: "+str(p))
artifacts=[dict(path=str(p),size=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in selected]
source=[]
for base in ("include","src","integrations","bindings","distribution","tests","tools","cmake"):
    for p in sorted((root/base).rglob("*")):
        if p.is_file() and "__pycache__" not in p.parts:source.append(dict(path=str(p.relative_to(root)),sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
for name in ("CMakeLists.txt","CMakePresets.json","LICENSE","NOTICE.md","README.md"):
    p=root/name;source.append(dict(path=name,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
for p in sorted((root/"android").rglob("*")):
    relative=p.relative_to(root/"android")
    if p.is_file() and not set(relative.parts).intersection({"build",".gradle",".cxx"}) and p.name!="local.properties":
        source.append(dict(path=str(p.relative_to(root)),sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
status=dict(project="MeguvizMedia",profile="bil-rife-ncnn-android",status="BLOCKED",contract_version="0.1",completed_scope="HOST_AND_ANDROID_CPU_REFERENCE_BUILD",bil_qualified=False,gpu_qualified=False,device_executed=False,
    stages=[
        dict(id="M0",status="DONE",result="PASS",scope="SOURCE_BASELINE_HOST_AND_ANDROID_BUILD",evidence=[str(selected[0]),str(selected[1]),str(package)]),
        dict(id="M1",status="BLOCKED",completed_subtasks=["C ABI/frame/clock/epoch","bounded async identity","Kotlin oracle normal/delayed parity","installed external C consumer","real v4.6 FP32 full-graph adapter and endpoint parity","one host DSO closure and approved exports","ARM64/ARMv7 API27 static SDK","API-only/JNI AAR, explicit loading, independent processing APK","same-byte identity native-payload, ELF/zip 16KB checks","bounded descriptor prefix copies and real page-boundary regression"],gates=[dict(id="M1.descriptor_compatibility",result="PASS",evidence=[str(compatibility)]),dict(id="M1.host_reference",result="PASS",evidence=[str(rife),str(selected[2])]),dict(id="M1.android_build_package",result="PASS",evidence=[str(package)]),dict(id="M1.android_device_identity",result="INCONCLUSIVE",reason="No device execution evidence"),dict(id="M1.legacy_bil_host_bridge",result="INCONCLUSIVE",reason="Full production JNI/HostBridge/Surface/audio integration belongs to I0 and remains incomplete")]),
        dict(id="M2",status="BLOCKED",requires=["C2","R2","M1.device"],result="INCONCLUSIVE",reason="No qualified Compute/RIFE AHB_HOST_WAIT provider or real EGL/Vulkan memory/lease evidence; new platform path not implemented"),
        dict(id="M3",status="BLOCKED",requires=["M2","C3"],result="INCONCLUSIVE",reason="No enabled/probed sync_fd bridge or device retirement evidence"),
        dict(id="M4",status="BLOCKED",requires=["M2"],result="INCONCLUSIVE",reason="Existing timeline was preserved; no new scheduling optimization or actual-present A/B claim"),
        dict(id="M5",status="BLOCKED",requires=["M3","M4","C5"],result="INCONCLUSIVE",reason="MediaCodec playback sample, multi-device/16KB runtime/long-run/AV qualification remain incomplete"),
        dict(id="M6",status="BLOCKED",requires=["M5","C6","I4","R5","S5"],result="INCONCLUSIVE",reason="No official Android RIFE/ncnn or bil-qualified release candidate; current packages are isolated identity/reference artifacts")],
    commands=[str(root/"artifacts/M1/commands.jsonl"),str(rife.parent/"commands.jsonl"),str(root/"artifacts/M1/android-commands.jsonl"),str(root/"artifacts/M1/android-package/commands.json")],
    artifacts=artifacts,source_manifest="artifacts/source-manifest.json",descriptor_revalidation_manifest="artifacts/M1/descriptor-compatibility/manifest.json",
    measurements=dict(service_latency="INCONCLUSIVE",actual_present_rate="INCONCLUSIVE",deadline_miss="INCONCLUSIVE",visible_latency="INCONCLUSIVE",energy="INCONCLUSIVE",av_sync="INCONCLUSIVE"),
    rollback="bil production path remains untouched by Media. Disable MGM_WITH_RIFE for independent identity-only consumer; no unqualified GPU path enabled.",
    known_failed_attempts=[dict(evidence="../coordination/review/abi-boundaries/unoptimized-results.json",cause="Original independent consumer observed 0xc0000005 before descriptor-copy repair; preserved unchanged; new real-page-boundary tests PASS"),dict(evidence="artifacts/M1/descriptor-compatibility/android-sdk-permission-failure.json",cause="Default execution could not launch installed SDK CMake (WinError 5); exact authorized toolchain rerun with execution permission PASS"),dict(evidence="artifacts/M1/rife-20261028T074258132039Z/adapter.stderr.txt",cause="Explicit allocation IDs collided with auto allocation; fixed by Compute owner; original adapter case rerun PASS"),dict(evidence="artifacts/M1/android-gradle-failed-sourceset.stderr.txt",cause="AGP sourceSets cast; switched to variant.sources API"),dict(evidence="artifacts/M1/android-gradle-failed-prefix.stderr.txt",cause="Cross-package root search; fixed explicit target-specific installed package lookup")],
    next_actions=["Independent source/artifact/raw-evidence review using prompts/06_INDEPENDENT_REVIEW.md","Implement and qualify Compute C2/C3 and RIFE R2 with real Android GPU devices","Connect OES/AHB/GLES platform chain with full guards and physical lease retirement","Complete I0 host JNI/cache/Surface/audio bridge without enabling an unqualified path","Run actual device identity, independent MediaCodec playback, compatibility and sustained AV/quality/performance gates"])
(root/"artifacts/source-manifest.json").write_text(json.dumps(source,indent=2)+"\n",encoding="utf-8")
(root/"STATUS.json").write_text(json.dumps(status,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
print("Media stage status and exact source/artifact identities recorded")
