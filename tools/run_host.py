import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
work=root.parent
vs=Path(r"C:\Program Files\Microsoft Visual Studio\18\Community")
cmake=vs/"Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
ninja=vs/"Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
vc=vs/"VC/Auxiliary/Build/vcvars64.bat"
logs=root/"artifacts/M1"
logs.mkdir(parents=True,exist_ok=True)
build=root/"build/host-reference"
prefix=logs/"install"
records=[]
# Capture only inside this subprocess; never print or persist the environment.
envproc=subprocess.run('cmd.exe /d /s /c "call "'+str(vc)+'" >nul && set"',stdout=subprocess.PIPE,stderr=subprocess.PIPE)
if envproc.returncode: raise RuntimeError("MSVC environment unavailable")
env=os.environ.copy()
for line in envproc.stdout.decode("mbcs",errors="replace").splitlines():
    if "=" in line and not line.startswith("="):
        key,value=line.split("=",1);env[key]=value
env["PATH"]=str(ninja.parent)+os.pathsep+str(cmake.parent)+os.pathsep+env.get("PATH",env.get("Path",""))
def run(name,argv,cwd=root):
    start=datetime.datetime.now(datetime.timezone.utc).isoformat()
    p=subprocess.run(list(map(str,argv)),cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    (logs/(name+".stdout.txt")).write_bytes(p.stdout);(logs/(name+".stderr.txt")).write_bytes(p.stderr)
    records.append(dict(name=name,cwd=str(cwd),argv=list(map(str,argv)),start_utc=start,end_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),exit_code=p.returncode,stdout=name+".stdout.txt",stderr=name+".stderr.txt"))
    (logs/"commands.jsonl").write_text("".join(json.dumps(x)+"\n" for x in records),encoding="utf-8")
    print(name,p.returncode,flush=True)
    if p.returncode:raise RuntimeError(name+" failed; inspect raw logs")
    return p.stdout
run("configure",[cmake,"-S",root,"-B",build,"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DMGM_COMPUTE_SOURCE="+str(work/"MeguvizCompute"),"-DMGC_WITH_NCNN=OFF"])
run("build",[cmake,"--build",build,"-j","4"])
run("ctest",[cmake.parent/"ctest.exe","--test-dir",build,"--output-on-failure"])
(logs/"ctest-details.stdout.txt").write_bytes((build/"Testing/Temporary/LastTest.log").read_bytes())
run("install",[cmake,"--install",build,"--prefix",prefix])
external=root/"build/external"
run("external_configure",[cmake,"-S",root/"examples/external_consumer","-B",external,"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DCMAKE_PREFIX_PATH="+str(prefix)])
run("external_build",[cmake,"--build",external])
run("external_run",[external/"media_external.exe"])
source=Path(r"D:\Bilibili_Innocent_Lab\Bilibili_Innocent_Lab\app\src\main\java\com\Bilibili_Innocent_Lab\xposedmodule\hook\feature\interpolation")
cache=Path.home()/".gradle/caches/modules-2/files-2.1"
def jar(group,name,version):return next((cache/group/name/version).rglob(name+"-"+version+".jar"))
compiler=jar("org.jetbrains.kotlin","kotlin-compiler-embeddable","2.4.21")
stdlib=jar("org.jetbrains.kotlin","kotlin-stdlib","2.4.21")
script=jar("org.jetbrains.kotlin","kotlin-script-runtime","2.4.21")
annotations=jar("org.jetbrains","annotations","13.0")
coroutines=jar("org.jetbrains.kotlinx","kotlinx-coroutines-core-jvm","1.10.2")
compiler_cp=os.pathsep.join(map(str,[compiler,stdlib,script,annotations,coroutines]))
oracle=build/"oracle.jar"
run("kotlin_oracle_compile",["java","-cp",compiler_cp,"org.jetbrains.kotlin.cli.jvm.K2JVMCompiler","-no-stdlib","-no-reflect","-classpath",os.pathsep.join(map(str,[stdlib,annotations])),"-d",oracle,source/"FrameTimeline.kt",source/"FrameRateTarget.kt",root/"tests/oracle/TimelineOracle.kt"])
for mode in ("normal","delayed"):
    extra=["--delayed"] if mode=="delayed" else []
    actual=run("cpp_"+mode,[build/"mgm_trace_replay.exe",*extra])
    expected=run("kotlin_"+mode,["java","-cp",str(oracle)+os.pathsep+str(stdlib),"com.Bilibili_Innocent_Lab.xposedmodule.hook.feature.interpolation.TimelineOracleKt",*extra])
    if actual.replace(b"\r\n",b"\n")!=expected.replace(b"\r\n",b"\n"):
        raise RuntimeError(mode+" timeline differs from original Kotlin oracle")
manifest=[]
for p in sorted(logs.rglob("*")):
    if p.is_file() and p.name!="manifest.json":manifest.append(dict(path=str(p.relative_to(root)),size=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
(logs/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
baseline=root/"artifacts/M0";baseline.mkdir(parents=True,exist_ok=True)
sources=[]
for p in (source/"FrameTimeline.kt",source/"FrameRateTarget.kt"):
    sources.append(dict(path=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
(baseline/"source-inventory.json").write_text(json.dumps(dict(source_repo="D:/Bilibili_Innocent_Lab",source_commit=subprocess.check_output(["git","-c","safe.directory=D:/Bilibili_Innocent_Lab","-C","D:/Bilibili_Innocent_Lab","rev-parse","HEAD"],text=True).strip(),files=sources,oracle_jar_sha256=hashlib.sha256(oracle.read_bytes()).hexdigest(),kotlin_compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest()),indent=2)+"\n",encoding="utf-8")
print("Media host identity, timeline oracle and installed external C consumer PASS",flush=True)
