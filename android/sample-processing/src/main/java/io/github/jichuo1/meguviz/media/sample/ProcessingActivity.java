package io.github.jichuo1.meguviz.media.sample;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import io.github.jichuo1.meguviz.media.MediaRuntime;
import java.util.Arrays;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class ProcessingActivity extends Activity {
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private volatile boolean closed;

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        TextView status = new TextView(this);
        status.setText("正在验证图像处理…");
        ImageView image = new ImageView(this);
        image.setAdjustViewBounds(true);
        layout.addView(status); layout.addView(image); setContentView(layout);
        worker.execute(() -> {
            try {
                byte[] input = new byte[128 * 128 * 3];
                for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) {
                    int offset = (y * 128 + x) * 3;
                    input[offset] = (byte)(x * 2); input[offset + 1] = (byte)(y * 2); input[offset + 2] = (byte)128;
                }
                MediaRuntime.loadPackagedIdentity();
                byte[] output = MediaRuntime.identityRgb(input, 128, 128, 128 * 3);
                if (!Arrays.equals(input, output)) throw new IllegalStateException("图像字节不一致");
                int[] pixels = new int[128 * 128];
                for (int i = 0; i < pixels.length; i++) pixels[i] = 0xff000000 | ((output[i * 3] & 255) << 16) | ((output[i * 3 + 1] & 255) << 8) | (output[i * 3 + 2] & 255);
                Bitmap bitmap = Bitmap.createBitmap(pixels, 128, 128, Bitmap.Config.ARGB_8888);
                runOnUiThread(() -> { if (!closed) { image.setImageBitmap(bitmap); status.setText("Identity 处理完成，图像字节一致（CPU reference）"); } });
            } catch (Exception | LinkageError error) {
                runOnUiThread(() -> { if (!closed) status.setText("处理失败：" + error.getClass().getSimpleName()); });
            }
        });
    }

    @Override public void onDestroy() { closed = true; worker.shutdown(); super.onDestroy(); }
}
