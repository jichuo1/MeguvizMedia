plugins { id("com.android.application") }
android {
    namespace = "io.github.jichuo1.meguviz.media.sample"
    compileSdk = 37
    defaultConfig { applicationId = "io.github.jichuo1.meguviz.media.sample"; minSdk = 27; targetSdk = 37; versionCode = 1; versionName = "0.1.0-reference" }
    compileOptions { sourceCompatibility = JavaVersion.VERSION_17; targetCompatibility = JavaVersion.VERSION_17 }
}
dependencies { implementation(project(":media-jni")) }
