package io.github.jichuo1.meguviz.media;

import android.os.Looper;
import java.io.File;
import java.io.IOException;

/** Explicit loading supports both packaged JNI and host-validated payloads. */
public final class MediaRuntime {
    private static volatile boolean attached;
    private MediaRuntime() {}

    public static synchronized void loadPackagedIdentity() {
        System.loadLibrary("meguviz_media_identity");
        attachAlreadyLoaded();
    }

    /** The host validates artifact size/hash/receipt before invoking this method. */
    public static synchronized void loadValidatedAbsolutePath(String library) throws IOException {
        File file = new File(library);
        if (!file.isAbsolute() || !file.getCanonicalPath().equals(library) || !file.isFile()) {
            throw new IOException("Expected an existing canonical native library path");
        }
        System.load(library);
        attachAlreadyLoaded();
    }

    public static synchronized void attachAlreadyLoaded() {
        if (MediaNative.abiVersion() != 1) throw new LinkageError("Unsupported Media ABI");
        attached = true;
    }

    /** CPU/reference processing; call from a worker. Native output is a new array. */
    public static byte[] identityRgb(byte[] rgb, int width, int height, int stride) {
        if (!attached) throw new IllegalStateException("Attach the validated native library first");
        if (Looper.myLooper() == Looper.getMainLooper()) throw new IllegalStateException("Use a worker for frame processing");
        return MediaNative.identityRgb(rgb, width, height, stride);
    }
}
