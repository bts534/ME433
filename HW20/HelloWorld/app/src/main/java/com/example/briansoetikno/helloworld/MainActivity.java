package com.example.briansoetikno.helloworld;

import android.Manifest;
import android.app.Activity;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.hoho.android.usbserial.driver.CdcAcmSerialDriver;
import com.hoho.android.usbserial.driver.ProbeTable;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import static android.graphics.Color.blue;
import static android.graphics.Color.green;
import static android.graphics.Color.red;
import static android.graphics.Color.rgb;


public class MainActivity extends Activity implements TextureView.SurfaceTextureListener {

    SeekBar seekBar1;
    SeekBar seekBar2;

    TextView myTextView;

    Button button;
    TextView myTextView2;
    ScrollView myScrollView;
    TextView myTextView3;

    private UsbManager manager;
    private UsbSerialPort sPort;
    private final ExecutorService mExecutor = Executors.newSingleThreadExecutor();
    private SerialInputOutputManager mSerialIoManager;

    private Camera mCamera;
    private TextureView mTextureView;
    private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder;
    private Bitmap bmp = Bitmap.createBitmap(640, 480, Bitmap.Config.ARGB_8888);
    private Canvas canvas = new Canvas(bmp);
    private Paint paint1 = new Paint();
    private TextView mTextView;
    private SeekBar seekBar;
    private int threshold;
    private SeekBar tiltBar;
    private int MAX_DUTY = 1199/3;
    private int left;
    private int right;
    private double kp=1;
    private int centerLocAvgPrev=320;

    static long prevtime = 0; // for FPS calculation

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); // keeps the screen from turning off

        seekBar1 = (SeekBar) findViewById(R.id.seek1);
        seekBar2 = (SeekBar) findViewById(R.id.seek2);

        mTextView = (TextView) findViewById(R.id.cameraStatus);
        seekBar = (SeekBar) findViewById(R.id.seekBar);
        threshold = (int) (seekBar.getProgress() );
        tiltBar = (SeekBar)  findViewById(R.id.seekBar2);

        // see if the app has permission to use the camera
        ActivityCompat.requestPermissions(MainActivity.this, new String[]{Manifest.permission.CAMERA}, 1);
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            mSurfaceView = (SurfaceView) findViewById(R.id.surfaceview);
            mSurfaceHolder = mSurfaceView.getHolder();

            mTextureView = (TextureView) findViewById(R.id.textureview);
            mTextureView.setSurfaceTextureListener(this);

            // set the paintbrush for writing text on the image
            paint1.setColor(0xffff0000); // red
            paint1.setTextSize(24);

            mTextView.setText("started camera");
        } else {
            mTextView.setText("no camera permissions");
        }

        setSeekBarListener();

        seekBar1.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                updateProgressTextView();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        seekBar2.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                updateProgressTextView();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        myTextView = (TextView) findViewById(R.id.textView01);
        myTextView.setText("Enter whatever you Like!");


        myTextView2 = (TextView) findViewById(R.id.textView02);
        myScrollView = (ScrollView) findViewById(R.id.ScrollView01);
        myTextView3 = (TextView) findViewById(R.id.textView03);
        button = (Button) findViewById(R.id.button1);

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                writeMotorCommands();
            }

        });

        manager = (UsbManager) getSystemService(Context.USB_SERVICE);
    }

    public void setSeekBarListener(){
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                threshold = (int) (progress );
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }

    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        mCamera = Camera.open();
        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPreviewSize(640, 480);
        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY); // no autofocusing
        parameters.setAutoExposureLock(true); // keep the white balance constant
        mCamera.setParameters(parameters);
        mCamera.setDisplayOrientation(90); // rotate to portrait mode

        try {
            mCamera.setPreviewTexture(surface);
            mCamera.startPreview();
        } catch (IOException ioe) {
            // Something bad happened
        }
    }

    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        // Ignored, Camera does all the work for us
    }

    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        mCamera.stopPreview();
        mCamera.release();
        return true;
    }

    // the important function
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        // every time there is a new Camera preview frame
        mTextureView.getBitmap(bmp);
        int massDev = 0;
        int numDev = 0;

        int centerLocAvg=0;
        int centerLocAvgCount=0;

        final Canvas c = mSurfaceHolder.lockCanvas();
        if (c != null) {
            int[] pixels = new int[bmp.getWidth()]; // pixels[] is the RGBA data
            int startY = 200; // which row in the bitmap to analyze to read


            int grayValue = 0;
            for (int j = 0; j < bmp.getHeight(); j+=1) {

                int sumTotal=0;
                int sumTotal2 = 0;

                bmp.getPixels(pixels, 0, bmp.getWidth(), 0, j, bmp.getWidth(), 1);

                for (int i = 0; i < bmp.getWidth(); i++) {
                    grayValue = (green(pixels[i])+red(pixels[i])+blue(pixels[i]))/3;
                    if (grayValue > threshold) {
                        if (j % 5 == 0)
                            pixels[i] = rgb(0, 255, 0); // over write the pixel with pure green
                        sumTotal += i ;
                        sumTotal2 ++;
                    }

                }

                int centerLoc = 0;

                if (sumTotal2 > 0) {
                    centerLoc = sumTotal / sumTotal2;


                    massDev += centerLoc - 640/2;
                    numDev ++;

                    pixels[centerLoc] = rgb(255, 0, 0);

                    if (j > 240-240 && j < 240+0) {
                        centerLocAvg +=centerLoc;
                        centerLocAvgCount++;
                    }
                } else {
                    centerLoc = bmp.getWidth() / 2;
                }



                // update the row
                bmp.setPixels(pixels, 0, bmp.getWidth(), 0, j, bmp.getWidth(), 1);
            }


        }

        if (centerLocAvgCount < 5)
        {
            centerLocAvg = centerLocAvgPrev;
        } else {
            centerLocAvg = centerLocAvg/centerLocAvgCount;
        }
        centerLocAvgPrev = centerLocAvg;

//        // draw a circle at some position
//        int pos = 50;
//        canvas.drawCircle(pos, 240, 5, paint1); // x position, y position, diameter, color
//
//        // write the pos as text
//        canvas.drawText("pos = " + pos, 10, 200, paint1);
        c.drawBitmap(bmp, 0, 0, null);
        mSurfaceHolder.unlockCanvasAndPost(c);

        calculateError(centerLocAvg, 640/2);
        writeMotorCommands();

        if (numDev > 0) {
            //tiltBar.setProgress(100);
            tiltBar.setProgress((int) ((double) massDev/(numDev*640.0/2)*50)+50);
        } else {
            tiltBar.setProgress(0);
        }

        // calculate the FPS to see how fast the code is running
        long nowtime = System.currentTimeMillis();
        long diff = nowtime - prevtime;
        mTextView.setText("FPS " + 1000 / diff + " Threshold: " + threshold);
        prevtime = nowtime;
    }

    private void updateProgressTextView() {
        int progressChanged1 = (int) ((seekBar1.getProgress()-50)/100.0*2*1199);
        int progressChanged2 = (int) ((seekBar2.getProgress()-50)/100.0*2*1199);


        myTextView.setText("Motor 1: "+ progressChanged1 + " Motor 2: " + progressChanged2);
    }

    private void calculateError(int value, int compareValue) {

        int error = value - compareValue; // 240 means the dot is in the middle of the screen
        if (error<0) { // slow down the left motor to steer to the left
            error  = -error;
            left = (int)(MAX_DUTY - kp*error);
            right = MAX_DUTY;
            if (left < 0){
                left = 0;
            }
        }
        else { // slow down the right motor to steer to the right
            right = (int) (MAX_DUTY - kp*error);
            left = MAX_DUTY;
            if (right<0) {
                right = 0;
            }
        }
    }

    private void writeMotorCommands() {
        String[] sendStrings = new String[4];

        // Convert to motorized PWM time
        //int progressChanged1 = (int) ((seekBar1.getProgress() - 50) / 100.0 * 2 * 1199);
        //int progressChanged2 = (int) ((seekBar2.getProgress() - 50) / 100.0 * 2 * 1199);

//        // send the direction to the motor
//        if (progressChanged1 < 0) {
//            sendStrings[0] = "0\n";
//            progressChanged1 = -progressChanged1; // prevent sending negative number
//        } else {
//            sendStrings[0] = "1\n";
//        }
//
//        // send the amplitude
//        sendStrings[1] = String.valueOf(progressChanged1) + '\n';
//
//        if (progressChanged2 < 0) {
//            sendStrings[2] = "0\n";
//            progressChanged2 = -progressChanged2; // prevent sending negative number
//        } else {
//            sendStrings[2] = "1\n";
//        }
//
//        sendStrings[3] = String.valueOf(progressChanged2) + '\n';

//        // send the four strings
//        for (int i = 0; i < sendStrings.length; i++) {
//            try {
//                sPort.write(sendStrings[i].getBytes(), 10); // 10 is the timeout
//                try{ Thread.sleep(250); }catch(InterruptedException e){ }
//            } catch (IOException e) {
//            }
//        }

        String sendString = String.valueOf(right) + ' ' + String.valueOf(left) + '\n';
        try {
            sPort.write(sendString.getBytes(), 10); // 10 is the timeout

        } catch (IOException e) {
        }
    }



    private final SerialInputOutputManager.Listener mListener =
            new SerialInputOutputManager.Listener() {
                @Override
                public void onRunError(Exception e) {

                }

                @Override
                public void onNewData(final byte[] data) {
                    MainActivity.this.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            MainActivity.this.updateReceivedData(data);
                        }
                    });
                }
            };

    @Override
    protected void onPause(){
        super.onPause();
        stopIoManager();
        if(sPort != null){
            try{
                sPort.close();
            } catch (IOException e){ }
            sPort = null;
        }
        finish();
    }

    @Override
    protected void onResume() {
        super.onResume();

        ProbeTable customTable = new ProbeTable();
        customTable.addProduct(0x04D8,0x000A, CdcAcmSerialDriver.class);
        UsbSerialProber prober = new UsbSerialProber(customTable);

        final List<UsbSerialDriver> availableDrivers = prober.findAllDrivers(manager);

        if(availableDrivers.isEmpty()) {
            //check
            return;
        }

        UsbSerialDriver driver = availableDrivers.get(0);
        sPort = driver.getPorts().get(0);

        if (sPort == null){
            //check
        }else{
            final UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
            UsbDeviceConnection connection = usbManager.openDevice(driver.getDevice());
            if (connection == null){
                //check
                PendingIntent pi = PendingIntent.getBroadcast(this, 0, new Intent("com.android.example.USB_PERMISSION"), 0);
                usbManager.requestPermission(driver.getDevice(), pi);
                return;
            }

            try {
                sPort.open(connection);
                sPort.setParameters(9600, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);

            }catch (IOException e) {
                //check
                try{
                    sPort.close();
                } catch (IOException e1) { }
                sPort = null;
                return;
            }
        }
        onDeviceStateChange();
    }

    private void stopIoManager(){
        if(mSerialIoManager != null) {
            mSerialIoManager.stop();
            mSerialIoManager = null;
        }
    }

    private void startIoManager() {
        if(sPort != null){
            mSerialIoManager = new SerialInputOutputManager(sPort, mListener);
            mExecutor.submit(mSerialIoManager);
        }
    }

    private void onDeviceStateChange(){
        stopIoManager();
        startIoManager();
    }

    private void updateReceivedData(byte[] data) {
        //do something with received data

        //for displaying:
        String rxString = null;
        try {
            rxString = new String(data, "UTF-8"); // put the data you got into a string
            myTextView3.append(rxString);
            myScrollView.fullScroll(View.FOCUS_DOWN);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
    }
}
