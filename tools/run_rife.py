import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

root=Path(__file__).resolve().parents[1]
vs=Path(r"C:\Program Files\Microsoft Visual Studio\18\Community")
cmake=vs/"Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
ninja=vs/"Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
vc=vs/"VC/Auxiliary/Build/vcvars64.bat"
evidence=root/"artifacts/M1"/("rife-"+datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ"))
evidence.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();p=subprocess.run('cmd.exe /d /s /c "call "'+str(vc)+'" >nul && set"',stdout=subprocess.PIPE,stderr=subprocess.PIPE)
if p.returncode:raise RuntimeError("MSVC environment unavailable")
for line in p.stdout.decode("mbcs",errors="replace").splitlines():
    if "=" in line and not line.startswith("="):
        key,value=line.split("=",1);env[key]=value
env["PATH"]=str(ninja.parent)+os.pathsep+str(cmake.parent)+os.pathsep+env.get("PATH",env.get("Path",""))
records=[]
def run(name,argv,cwd=root):
    start=datetime.datetime.now(datetime.timezone.utc).isoformat();p=subprocess.run(list(map(str,argv)),cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    (evidence/(name+".stdout.txt")).write_bytes(p.stdout);(evidence/(name+".stderr.txt")).write_bytes(p.stderr)
    records.append(dict(name=name,cwd=str(cwd),argv=list(map(str,argv)),start_utc=start,end_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),exit_code=p.returncode))
    (evidence/"commands.jsonl").write_text("".join(json.dumps(x)+"\n" for x in records),encoding="utf-8")
    print(name,p.returncode,flush=True)
    if p.returncode:raise RuntimeError(name+" failed; inspect "+str(evidence))
    return p.stdout
compute=root.parent/"MeguvizCompute/build/install/windows-cpu"
rife=root.parent/"MeguvizRIFE/build/install/windows-reference"
prefix=evidence/"install"
build=root/"build/host-rife"
run("configure",[cmake,"-S",root,"-B",build,"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DMGM_WITH_RIFE=ON","-DCMAKE_PREFIX_PATH="+str(compute)+";"+str(rife)])
run("build",[cmake,"--build",build,"-j","4"])
run("ctest",[cmake.parent/"ctest.exe","--test-dir",build,"--output-on-failure"])
run("adapter",[build/"mgm_rife_adapter_test.exe",r"D:\Bilibili_Innocent_Lab\runtime\player-frame-interpolation-20261018\dependency-research\rife\models\rife-v4.6",root.parent/"MeguvizRIFE/models/catalog/rife46.manifest.json"])
run("install",[cmake,"--install",build,"--prefix",prefix])
headers=[*compute.joinpath("include/meguviz").glob("*.h"),*rife.joinpath("include/meguviz").glob("*.h"),*prefix.joinpath("include/meguviz").glob("*.h")]
exports={"mgm_composition_fixture_abi"}
for header in headers:
    exports.update(re.findall(r"\b(mg[cmr]_[A-Za-z0-9_]+)\s*\(",header.read_text()))
definition=evidence/"approved-exports.def";definition.write_text("EXPORTS\n"+"\n".join(sorted(exports))+"\n",encoding="utf-8")
composition=root/"build/composition-host"
run("composition_configure",[cmake,"-S",root/"distribution/host","-B",composition,"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DCMAKE_PREFIX_PATH="+str(compute)+";"+str(rife)+";"+str(prefix),"-DMGM_APPROVED_EXPORTS="+str(definition)])
run("composition_build",[cmake,"--build",composition,"-j","4"])
dll=composition/"meguviz_composition_fixture.dll"
dumpbin=vs/"VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe"
dump=run("composition_exports",[dumpbin,"/exports",dll])
seen=set(re.findall(rb"\b(mg[cmr]_[A-Za-z0-9_]+)\b",dump))
if seen!={x.encode() for x in exports}:raise RuntimeError("actual exported symbols differ from approved list")
dependencies=run("composition_dependencies",[dumpbin,"/dependents",dll])
if b"ncnn.dll" in dependencies.lower() or b"meguviz_compute" in dependencies.lower() or b"meguviz_rife" in dependencies.lower():raise RuntimeError("unexpected second Meguviz/ncnn runtime DSO")
files=[dict(path=str(p),size=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in [dll,*headers,definition]]
(evidence/"summary.json").write_text(json.dumps(dict(result="PASS",scope="HOST_CPU_REFERENCE_COMPOSITION_FIXTURE",model_inference_adapter=True,endpoint_bitwise=True,approved_export_count=len(exports),single_meguviz_dso=True,android_qualified=False,bil_qualified=False,artifacts=files),indent=2)+"\n",encoding="utf-8")
(root/"artifacts/M1/latest-rife.json").write_text(json.dumps(dict(run=str(evidence),summary=str(evidence/"summary.json")),indent=2)+"\n",encoding="utf-8")
print("RIFE adapter and one-DSO host composition PASS: "+str(evidence),flush=True)
