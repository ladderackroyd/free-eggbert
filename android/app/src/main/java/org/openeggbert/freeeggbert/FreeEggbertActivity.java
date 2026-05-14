package org.openeggbert.freeeggbert;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import org.libsdl.app.SDLActivity;
/**
 * Free Eggbert Android entry point.
 *
 * This class simply extends SDLActivity so that:
 *   - The application appears in the launcher as "Free Eggbert".
 *   - The SDL3 Java glue (SDLActivity) handles all lifecycle, surface, input,
 *     and audio initialisation automatically.
 *
 * Native code lives in libmain.so (built from the game's C++ sources via CMake).
 * SDL3 discovers it by looking for a library named "main".
 *
 * Orientation is locked to sensorLandscape (both landscape sides allowed, portrait
 * blocked) here and in AndroidManifest.xml because Speedy Eggbert is a landscape game.
 */
public class FreeEggbertActivity extends SDLActivity {
    /**
     * Returns the name of the native shared library to load.
     * SDL3's Java glue calls this to find our game code.
     */
    @Override
    protected String[] getLibraries() {
        return new String[]{
                "SDL3",
                "main"
        };
    }
    /**
     * Force sensor-landscape at startup, before SDL has a chance to call setOrientationBis().
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        super.onCreate(savedInstanceState);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
    }
    /**
     * Re-enforce landscape every time the activity resumes (e.g. after returning
     * from the background, where SDL may have reset orientation).
     * Also re-applies immersive full-screen mode.
     */
    @Override
    protected void onResume() {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        super.onResume();
        hideSystemBars();
    }
    /**
     * Re-enforce landscape if Android fires a configuration change (e.g. keyboard
     * attached, screen size changed) that could allow a rotation transition.
     */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        super.onConfigurationChanged(newConfig);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
    }
    /**
     * Override the SDL JNI-callable orientation setter so that SDL internal logic
     * cannot override our forced landscape lock.
     */
    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
    }
    /**
     * Also apply on window focus gained, because Android can re-show the system
     * bars whenever the window loses and regains focus.
     */
    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            hideSystemBars();
        }
    }
    /** Hide status bar and navigation bar using the appropriate API level. */
    private void hideSystemBars() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // API 30+ (Android 11+): WindowInsetsController
            WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.systemBars());
                controller.setSystemBarsBehavior(
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            // API < 30: legacy system UI visibility flags
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            );
        }
    }
}
