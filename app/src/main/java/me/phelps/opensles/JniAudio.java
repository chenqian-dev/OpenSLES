package me.phelps.opensles;

/**
 * Created by User on 2016/11/8.
 */

public class JniAudio {

    public static native int createEngine();
    public static native int createRecorder();
    public static native int startRecording();
    public static native int stopRecording();
    public static native int destoryEngine();

}
