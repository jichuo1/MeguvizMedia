package io.github.jichuo1.meguviz.media;

final class MediaNative {
    private MediaNative() {}
    static native int abiVersion();
    static native byte[] identityRgb(byte[] rgb, int width, int height, int stride);
}
