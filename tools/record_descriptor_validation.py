"""Record the current descriptor repair evidence without executing target code."""
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
out=root/"artifacts/M1/descriptor-compatibility"
out.mkdir(parents=True,exist_ok=True)
read=lambda p:json.loads(p.read_text(encoding="utf-8-sig"))
identity=lambda p:dict(path=str(p.relative_to(root)) if p.is_relative_to(root) else str(p),size=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest())
host=root/"artifacts/M1/commands.jsonl"
rife=Path(read(root/"artifacts/M1/latest-rife.json")["run"])
android=root/"artifacts/M1/android-commands.jsonl"
package=root/"artifacts/M1/android-package/manifest.json"
gradle=root/"artifacts/M1/android-gradle-command.json"
for path in (host,rife/"commands.jsonl",android):
    commands=[json.loads(line) for line in path.read_text().splitlines() if line]
    if not commands or any(c["exit_code"] for c in commands):raise RuntimeError("nonpassing command evidence: "+str(path))
if read(gradle)["exit_code"] or read(package)["result"]!="PASS":raise RuntimeError("Android package gate did not pass")
if '"minimum_prefix_success":true' not in (root/"artifacts/M1/ctest-details.stdout.txt").read_text():raise RuntimeError("missing real-page-boundary test result")
if read(rife/"summary.json")["result"]!="PASS":raise RuntimeError("RIFE composition gate did not pass")
source_names=("src/descriptor.h","src/core/media.cpp","src/core/timeline.cpp","integrations/rife/adapter.cpp","include/meguviz/media.h","include/meguviz/timeline.h","tests/abi_guard_pages.cpp","tests/descriptor_pages.h","tests/rife_adapter.cpp")
artifacts=[root/"artifacts/M1/install/lib/meguviz_media_core.lib",rife/"install/lib/meguviz_media_core.lib",rife/"install/lib/meguviz_media_rife_integration.lib",root/"build/composition-host/meguviz_composition_fixture.dll"]
for abi in ("arm64-v8a","armeabi-v7a"):
    artifacts.extend((root/("artifacts/M1/android-"+abi+"/install/lib/libmeguviz_media_core.a"),root/("artifacts/M1/android-package/native-payload/"+abi+"/libmeguviz_media_identity.so")))
artifacts.extend(Path(a["path"]) for a in read(package)["artifacts"])
evidence=[host,root/"artifacts/M1/ctest.stdout.txt",root/"artifacts/M1/ctest-details.stdout.txt",rife/"commands.jsonl",rife/"adapter.stdout.txt",rife/"ctest.stdout.txt",rife/"summary.json",android,gradle,root/"artifacts/M1/android-gradle.stdout.txt",package]
original=root.parent/"coordination/review/abi-boundaries/unoptimized-results.json"
summary=dict(result="PASS",scope="HOST_AND_ANDROID_CPU_REFERENCE_DESCRIPTOR_REVALIDATION",recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),input_contract="Read exactly the 8-byte header, validate size/major, copy only the declared prefix into a zero-initialized local descriptor, validate flags/reserved, then use the copy",older_prefix="Inputs through the last non-reserved field are supported; frame_describe also accepts a header-only caller; undersized mandatory prefixes are rejected",fixtures=["8-byte input descriptors end immediately before inaccessible pages","exact minimum prefix input and SPI output allocations","unknown major/required_flags and nonzero reserved rejection","future input tail ignored","identity accepted callback and ownership with a short request","RIFE short prepare/API/request with endpoint bitwise and alpha=0.5 inference","host identity/timeline/installed external C consumer and Kotlin normal/delayed parity","ARM64/ARMv7 API27 SDK rebuild","API-only/JNI AAR and processing APK rebuild; payload same bytes; approved exports; ELF/APK 16KB alignment"],sources=[identity(root/p) for p in source_names],artifacts=[identity(p) for p in artifacts],evidence=[identity(p) for p in evidence],preserved_before_revalidation="artifacts/M1/before-descriptor-revalidation-20261028T162947",original_failure=identity(original),model_copied_into_packages=False,device_executed=False,gpu_qualified=False,bil_qualified=False)
(out/"summary.json").write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
(out/"android-sdk-permission-failure.json").write_text(json.dumps(dict(result="LAUNCH_FAILED",scope="INFRASTRUCTURE_BEFORE_COMPILER_EXECUTION",observed_exception="PermissionError: [WinError 5] Access is denied",attempt="tools/build_android.py with the verified SDK CMake 4.1.2 and NDK 30.0.16248370 under default execution permissions",resolution="Reran the same authorized toolchain with execution permission; all six Android SDK configure/build/install commands PASS",model_or_device_operation=False),indent=2)+"\n",encoding="utf-8")
subprocess.run([sys.executable,str(root/"tools/refresh_status.py")],cwd=root,check=True)
# The manifest snapshots the completed status, sources, raw command records and
# built files. It deliberately omits itself to avoid a circular identity.
paths=[out/"summary.json",out/"android-sdk-permission-failure.json",root/"STATUS.json",root/"artifacts/source-manifest.json",*artifacts,*evidence]
for abi in ("arm64-v8a","armeabi-v7a"):paths.append(root/("artifacts/M1/android-"+abi+"/manifest.json"))
(out/"manifest.json").write_text(json.dumps([identity(p) for p in sorted(set(paths))],indent=2)+"\n",encoding="utf-8")
# Refresh the aggregate M1 manifest after all validation records are final.
manifest=[identity(p) for p in sorted((root/"artifacts/M1").rglob("*")) if p.is_file() and p.name!="manifest.json"]
(root/"artifacts/M1/manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
print("Descriptor compatibility source/artifact identities recorded: "+str(out/"summary.json"))
