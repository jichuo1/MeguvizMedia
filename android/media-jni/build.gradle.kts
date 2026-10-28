plugins { id("com.android.library") }
android {
    namespace = "io.github.jichuo1.meguviz.media.jni"
    compileSdk = 37
    ndkVersion = "30.0.16248370"
    defaultConfig {
        minSdk = 27
        ndk { abiFilters += listOf("arm64-v8a", "armeabi-v7a") }
        externalNativeBuild { cmake { arguments += listOf("-DANDROID_STL=c++_static", "-DMGM_IDENTITY_SDK_ROOT=${rootProject.projectDir.parentFile}/artifacts/M1") } }
    }
    externalNativeBuild { cmake { path = file("../../distribution/android/CMakeLists.txt"); version = "4.1.2" } }
    compileOptions { sourceCompatibility = JavaVersion.VERSION_17; targetCompatibility = JavaVersion.VERSION_17 }
}
androidComponents.onVariants { variant -> variant.sources.java?.addStaticSourceDirectory(file("../../bindings/android/src/main/java").absolutePath) }
val identitySdkFiles = fileTree("../../artifacts/M1") { include("android-*/install/**") }
tasks.configureEach {
    if (name.contains("CMake") || name.contains("externalNativeBuild")) inputs.files(identitySdkFiles).withPropertyName("meguvizIdentitySdk")
}
