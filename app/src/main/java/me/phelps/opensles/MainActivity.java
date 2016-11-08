package me.phelps.opensles;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.widget.TextView;

import butterknife.ButterKnife;
import butterknife.InjectView;

public class MainActivity extends AppCompatActivity {

    @InjectView(R.id.sample_text)
    TextView sampleText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ButterKnife.inject(this);

        JniAudio recorder = new JniAudio();
        sampleText.setText("from jni= "+recorder.createRecorder());

    }

    static {
        System.loadLibrary("native-audio");
    }
}
