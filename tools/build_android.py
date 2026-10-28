"""Cross-build the actual CPU/reference Media SDK; never runs target programs."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import subprocess

parser=argparse.ArgumentParser()
parser.add_argument("--ndk",required=True,type=Path)
parser.add_argument("--cmake",required=True,type=Path)
parser.add_argument("--ninja",required=True,type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
records=[]
for abi in ("arm64-v8a","armeabi-v7a"):
    build=root/"build"/("android-"+abi)
    evidence=root/"artifacts/M1"/("android-"+abi)
    evidence.mkdir(parents=True,exist_ok=True)
    prefix=evidence/"install"
    commands=[("configure",[args.cmake,"-S",root,"-B",build,"-G","Ninja","-DCMAKE_MAKE_PROGRAM="+str(args.ninja),"-DCMAKE_TOOLCHAIN_FILE="+str(args.ndk/"build/cmake/android.toolchain.cmake"),"-DANDROID_ABI="+abi,"-DANDROID_PLATFORM=android-27","-DANDROID_STL=c++_static","-DCMAKE_BUILD_TYPE=Release","-DMGM_BUILD_TESTS=OFF","-DMGC_WITH_NCNN=OFF","-DMGM_COMPUTE_SOURCE="+str(root.parent/"MeguvizCompute")]),("build",[args.cmake,"--build",build,"-j","4"]),("install",[args.cmake,"--install",build,"--prefix",prefix])]
    for name,argv in commands:
        start=datetime.datetime.now(datetime.timezone.utc).isoformat();p=subprocess.run(list(map(str,argv)),cwd=root,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        (evidence/(name+".stdout.txt")).write_bytes(p.stdout);(evidence/(name+".stderr.txt")).write_bytes(p.stderr)
        records.append(dict(abi=abi,name=name,argv=list(map(str,argv)),cwd=str(root),start_utc=start,end_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),exit_code=p.returncode,stdout=str(evidence/(name+".stdout.txt")),stderr=str(evidence/(name+".stderr.txt"))))
        (root/"artifacts/M1/android-commands.jsonl").write_text("".join(json.dumps(r)+"\n" for r in records),encoding="utf-8")
        print(abi,name,p.returncode,flush=True)
        if p.returncode:raise RuntimeError("cross build failed; inspect raw logs")
    files=[dict(path=str(p.relative_to(root)),size=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in sorted(prefix.rglob("*")) if p.is_file()]
    (evidence/"manifest.json").write_text(json.dumps(dict(scope="ANDROID_CPU_REFERENCE_CROSS_BUILD_ONLY",abi=abi,device_executed=False,ncnn_vulkan_qualified=False,artifacts=files),indent=2)+"\n",encoding="utf-8")
