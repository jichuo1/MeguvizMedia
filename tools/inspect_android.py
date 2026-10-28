import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import zipfile

parser=argparse.ArgumentParser();parser.add_argument("--ndk",type=Path,required=True);parser.add_argument("--zipalign",type=Path,required=True);args=parser.parse_args()
root=Path(__file__).resolve().parents[1];out=root/"artifacts/M1/android-package";out.mkdir(parents=True,exist_ok=True)
aar=root/"android/media-jni/build/outputs/aar/media-jni-release.aar"
api=root/"android/media-api/build/outputs/aar/media-api-release.aar"
apk=root/"android/sample-processing/build/outputs/apk/debug/sample-processing-debug.apk"
readelf=args.ndk/"toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-readelf.exe"
records=[]
def run(name,argv):
    p=subprocess.run(list(map(str,argv)),cwd=root,stdout=subprocess.PIPE,stderr=subprocess.PIPE);(out/(name+".stdout.txt")).write_bytes(p.stdout);(out/(name+".stderr.txt")).write_bytes(p.stderr);records.append(dict(name=name,argv=list(map(str,argv)),exit_code=p.returncode))
    if p.returncode:raise RuntimeError(name+" failed; inspect raw evidence")
    return p.stdout.decode("utf-8",errors="replace")
sha=lambda b:hashlib.sha256(b).hexdigest()
approved={"Java_io_github_jichuo1_meguviz_media_MediaNative_abiVersion","Java_io_github_jichuo1_meguviz_media_MediaNative_identityRgb"}
payload=[]
with zipfile.ZipFile(aar) as package,zipfile.ZipFile(api) as api_package:
    if any(n.endswith(".so") for n in api_package.namelist()):raise RuntimeError("API-only AAR contains a native owner")
    expected={"jni/"+abi+"/libmeguviz_media_identity.so" for abi in ("arm64-v8a","armeabi-v7a")}
    actual={n for n in package.namelist() if n.endswith(".so")}
    if actual!=expected:raise RuntimeError("JNI AAR native owner set differs from declared ABIs")
    jni_classes=zipfile.ZipFile(io.BytesIO(package.read("classes.jar")));api_classes=zipfile.ZipFile(io.BytesIO(api_package.read("classes.jar")))
    for cls in ("io/github/jichuo1/meguviz/media/MediaRuntime.class","io/github/jichuo1/meguviz/media/MediaNative.class"):
        if jni_classes.read(cls)!=api_classes.read(cls):raise RuntimeError("API/JNI class bytes differ")
    classes=out/"api-classes.jar";classes.write_bytes(api_package.read("classes.jar"))
    bytecode=run("api_bytecode",["javap","-classpath",classes,"-c","-p","io.github.jichuo1.meguviz.media.MediaRuntime","io.github.jichuo1.meguviz.media.MediaNative"])
    if "static {};" in bytecode:raise RuntimeError("Unexpected static initializer in explicit-loading API")
    for name in sorted(expected):
        data=package.read(name);abi=name.split("/")[1];path=out/"native-payload"/abi/"libmeguviz_media_identity.so";path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
        program=run(abi+"_program_headers",[readelf,"-lW",path]);symbols=run(abi+"_symbols",[readelf,"--dyn-syms","--wide",path]);dependencies=run(abi+"_dynamic",[readelf,"-dW",path])
        aligns=[int(line.split()[-1],16) for line in program.splitlines() if line.lstrip().startswith("LOAD")]
        if not aligns or any(n<16384 for n in aligns):raise RuntimeError("ELF LOAD is not 16KB aligned")
        defined=set()
        for line in symbols.splitlines():
            parts=line.split()
            if len(parts)>=8 and parts[4] in ("GLOBAL","WEAK") and parts[6]!="UND":defined.add(parts[7])
        if defined!=approved:raise RuntimeError("Unapproved JNI symbols: "+repr(defined))
        if "libc++_shared.so" in dependencies or "libncnn.so" in dependencies or "libmeguviz_compute" in dependencies:raise RuntimeError("Unexpected shared runtime dependency")
        payload.append(dict(abi=abi,path=str(path),source_entry=name,size=len(data),sha256=sha(data),source_aar_sha256=sha(aar.read_bytes()),elf_load_alignment=aligns,approved_exports=sorted(defined),aar_payload_bytes_equal=path.read_bytes()==package.read(name)))
run("apk_zip_alignment",[args.zipalign,"-c","-P","16","-v","4",apk])
with zipfile.ZipFile(apk) as z:
    natives=[n for n in z.namelist() if n.endswith(".so")]
    if set(natives)!={"lib/"+abi+"/libmeguviz_media_identity.so" for abi in ("arm64-v8a","armeabi-v7a")}:raise RuntimeError("Duplicate or missing native owner in sample APK")
(out/"commands.json").write_text(json.dumps(records,indent=2)+"\n",encoding="utf-8")
(out/"manifest.json").write_text(json.dumps(dict(result="PASS",profile="android-identity-cpu-reference",device_executed=False,bil_qualified=False,gpu_qualified=False,api_native_owner_count=0,jni_native_owner_count_per_abi=1,static_load_initializer=False,native_payload=payload,artifacts=[dict(path=str(p),size=p.stat().st_size,sha256=sha(p.read_bytes())) for p in (api,aar,apk)]),indent=2)+"\n",encoding="utf-8")
print("Android API/JNI AAR, exact payload bytes, ELF exports/alignment and APK zip alignment PASS")
