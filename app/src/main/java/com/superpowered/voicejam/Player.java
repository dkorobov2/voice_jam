package com.superpowered.voicejam;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.SoundPool;
import android.os.Build;
import android.util.Log;

import java.util.ArrayList;

/**
 * Created by korobov2 on 9/10/2017.
 */

public class Player {
    AssetFileDescriptor afd;
    MediaPlayer mPlayer = new MediaPlayer();
    private SoundPool pianoSounds;
    Context mainActivityContext;
    boolean ready;
    ArrayList<Integer> soundIds;

    // from C2 to B5
    String[] notes = {"C2", "Db2", "D2", "Eb2", "E2", "F2", "Gb2", "G2", "Ab2", "A2", "Bb2", "B2", "C3", "Db3", "D3", "Eb3", "E3", "F3", "Gb3", "G3", "Ab3", "A3", "Bb3", "B3", "C4", "Db4", "D4", "Eb4", "E4", "F4", "Gb4", "G4", "Ab4", "A4", "Bb4", "B4", "C5", "Db5", "D5", "Eb5", "E5", "F5", "Gb5", "G5", "Ab5", "A5", "Bb5", "B5",};

    public Player(Context c) {
        mainActivityContext = c;
        soundIds = new ArrayList<Integer>();
        pianoSounds = buildSoundPool();
        ready = false;

        for (int i = 0; i < notes.length; i++) {
            try {
                afd = mainActivityContext.getAssets().openFd("piano_ff_" + notes[i] + ".mp3");
                soundIds.add(pianoSounds.load(afd, i));
            } catch (Exception e) {
            }
        }

        pianoSounds.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
            @Override
            public void onLoadComplete(SoundPool soundPool, int mySoundId, int status) {
                ready = true;
            }
        });
    }

    /**
     * Verify device's API before to load soundpool
     *
     * @return
     */
    @SuppressWarnings("deprecation")
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private SoundPool buildSoundPool() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            AudioAttributes audioAttributes = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_UNKNOWN)
                    .setContentType(AudioAttributes.CONTENT_TYPE_UNKNOWN)
                    .build();

            pianoSounds = new SoundPool.Builder()
                    .setMaxStreams(24)
                    .setAudioAttributes(audioAttributes)
                    .build();
        } else {
            buildBeforeAPI21();
        }
        return pianoSounds;
    }

    public void buildBeforeAPI21() {
        pianoSounds = new SoundPool(24, AudioManager.STREAM_MUSIC, 0);
    }

    public void playNote(int index) {
        if (ready) {
            Log.i("player", "playing note");
            pianoSounds.play(soundIds.get(index), 1, 1, 0, 0, 1);
        }
    }
}
