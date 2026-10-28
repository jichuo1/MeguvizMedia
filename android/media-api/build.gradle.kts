plugins { id("com.android.library") }
android {
    namespace = "io.github.jichuo1.meguviz.media.api"
    compileSdk = 37
    defaultConfig { minSdk = 27 }
    compileOptions { sourceCompatibility = JavaVersion.VERSION_17; targetCompatibility = JavaVersion.VERSION_17 }
}
androidComponents.onVariants { variant -> variant.sources.java?.addStaticSourceDirectory(file("../../bindings/android/src/main/java").absolutePath) }
