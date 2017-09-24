package com.superpowered.voicejam;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.media.AudioManager;
import android.content.Context;
import android.os.Build;
import android.widget.Button;
import android.view.View;
import android.widget.Toast;

import static android.support.v4.content.PermissionChecker.PERMISSION_GRANTED;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = MainActivity.class.getSimpleName();
    private static final int MY_PERMISSION_REQUEST_RECORD_AUDIO = 101;
    private boolean permissionIsGranted = true;
    boolean recording = false;

    // variables for initializing PitchDetector
    private String samplerateString = null;
    private String buffersizeString = null;
    private String storePath;

    private Player player;

    /* Checks if external storage is available for read and write */
    public boolean isExternalStorageWritable() {
        String state = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(state)) {
            return true;
        }
        return false;
    }

    /* Checks if external storage is available to at least read */
    public boolean isExternalStorageReadable() {
        String state = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(state) ||
                Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
            return true;
        }
        return false;
    }

    void requestPermissions() {
        // request audio permission at runtime
        if (ActivityCompat.checkSelfPermission(MainActivity.this, Manifest.permission.RECORD_AUDIO) != PERMISSION_GRANTED) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                permissionIsGranted = false;
                requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, MY_PERMISSION_REQUEST_RECORD_AUDIO);
            }
        }
    }

    void initializeSuperpoweredParams() {
        if (Build.VERSION.SDK_INT >= 17) {
            AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
            samplerateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
            buffersizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        }
        if (samplerateString == null) samplerateString = "44100";
        if (buffersizeString == null) buffersizeString = "512";

        // get external storage path for PitchDetector
        if (isExternalStorageReadable() && isExternalStorageWritable()) {
            storePath = Environment.getExternalStorageDirectory().getAbsolutePath();
        }
        else {
            storePath = getFilesDir().getAbsolutePath();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        requestPermissions();

        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        initializeSuperpoweredParams();

        // Get the device's sample rate and buffer size to enable low-latency Android audio output, if available.
        Log.d(TAG, "Directory: " + storePath);
        Log.d(TAG, "Path: " + getPackageName());

        if(permissionIsGranted) {
            // Arguments: path to the APK file, offset and length of the two resource files, sample rate, audio buffer size.
            PitchDetector(Integer.parseInt(samplerateString), Integer.parseInt(buffersizeString), getPackageResourcePath(), storePath);
        }


        registerJNI();

        player = new Player(MainActivity.this);
        //Thread playerThread = new Thread(player);
        //playerThread.start();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            case MY_PERMISSION_REQUEST_RECORD_AUDIO: {
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    permissionIsGranted = true;
                    PitchDetector(Integer.parseInt(samplerateString), Integer.parseInt(buffersizeString), getPackageResourcePath(), storePath);
                }
                else {
                    permissionIsGranted = false;
                    Toast.makeText(MainActivity.this, "This app requires audio recording permission to be granted.", Toast.LENGTH_SHORT).show();
                }
            }
        }
    }

    //record button events
    public void PitchDetector_Start(View button) {
        if (permissionIsGranted) {
            recording = !recording;
            onStart(recording);
            Button startButton = (Button) findViewById(R.id.startButton);
            if (startButton != null) startButton.setText(recording ? "Stop" : "Record!");
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    void playNote(int index) {
        player.playNote(index);
        return ;
    }

    public native void registerJNI();

    private native void PitchDetector(int samplerate, int buffersize, String apkPath, String storePath);
    private native void onStart(boolean record);

    static {
        System.loadLibrary("PitchDetector");
    }
}
