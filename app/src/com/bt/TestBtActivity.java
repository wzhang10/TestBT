package com.bt;

import android.app.Activity;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class TestBtActivity extends Activity {
	private Button bRequestKey, bSendKeys, bTurnOnBluetooth;
	private TextView textView;
	Handler h;
	BluetoothKM bt;
	
	private final String testkey[] = {
"1111111231111111","0gersrehrth00000","00srgserh0000000","000dghdj00000000","00234t2300000000",
"0000000004325gh0","0000024f34g00000","0000000000srth00","000000cv000sdfg0","00000000sdrg0000",
"00000000srtbs000","0000000erb000000","0000000c00000000","000000vcv00f0000","000000cv00000000",
"0000000000fd0000","00000s0v000c0000","0000000000000000","0000cv00000sd000","00000000sd00f330",
"0000sd00d0000000","0000000c00000000","0000000vxc230000","00eg000000000000","00000000000nw000",
"000000d0s0000000","0000ss0v00x00000","0000000000000000","0000eh0000000000","00000cvwewef0000",
"0000000000000000","0000000000c00000","000000v230000000","000000wer0000000","0000kef000000000",
"0000ssd000000000","0000c00xc0000000","000000q0000wer00","000000000ge00000","000000000000w000",
"000000000dd00000","0000xd0000x00000","00000c0000000000","00000000000r0000","00000kj000ew0000",
"00000sd00000d000","0000000x00000000","00000c00000erg00","00000000000er000","000klk0000000000",
"00000000erg00000","00000qw00qwx0000","00000c000eg00000","00000000000er000","0000000000wr0000",
"000000s0d0dddf00","000000000c000000","000000cc00000000","00000000000er000","0000000000000000",
"0000f00000000000","0000000000000000","000000c000000000","0000000ghj000we0","00000000r0000000",
"000s00sdfsdf0000","000000svc0c00000","00000cc00c000000","000000000000000r","0000000e00000000",
"0000000000000000","0000000000000000","00000000xv000000","000000hiu0000000","000000er00000000",
"0000000dsf000000","000000c000x00000","00000c0000000000","000et00000000000","00dsfg0000000000",
"00000ssd00000000","0000c00000v00000","00000cx000c0xc00","00000wert0we0000","0000000cvxd00000",
"000000s000000000","000000c000c00000","0000000c00000000","00000000000fdsg0","000000sd00000000",
"000000s000000000","000000c000c00000","0000000c00000000","00000000000fdsg0","000000sd00000000",
"000000sdf0000000","0000000000vc0000","000x000c00v00000","0000000sdfg00000","000000qwe0000000"
	};
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        /********************************************
         * 											*
         * 				UI CODE						*
         * 											*
         ********************************************/
        setContentView(R.layout.main);
        bRequestKey = (Button) findViewById(R.id.bRequestKey);
        bSendKeys = (Button) findViewById(R.id.bSendKeys);
        bTurnOnBluetooth = (Button) findViewById(R.id.bTurnOnBluetooth);
        textView = (TextView) findViewById(R.id.textView);
        h = new Handler(){
        	@Override
        	public void handleMessage(Message msg) {
        		textView.setText((String)msg.obj);
        	}
        };

		bRequestKey.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				boolean ret = bt.requestKey();
				if(!ret)textView.setText("other task running");
			}
		});
		
		bSendKeys.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				byte data[][] = new byte[testkey.length][];
				for(int i=0;i<data.length;i++){
					data[i] = testkey[i].getBytes();
				}
				boolean ret = bt.sync(data);
				if(!ret)textView.setText("other task running");
			}
		});
		
		bTurnOnBluetooth.setOnClickListener(new OnClickListener() {			
			@Override
			public void onClick(View v) {
				bt.turnOnBluetooth();
			}
		});
		
		/********************************************
         * 											*
         * 				BT Related Code				*
         * 											*
         ********************************************/
		bt = new BluetoothKM() {
			@Override
			public void nomoreKey() {
				Message m = new Message();
				m.obj = "MCU has no more key";
				TestBtActivity.this.h.sendMessage(m);
			}
			
			@Override
			public void gotKey(byte[] key, int index) {
				Message m = new Message();
				m.obj = "Got key "+Integer.toString(index)+" "+ new String(key);
				TestBtActivity.this.h.sendMessage(m);
			}
			
			@Override
			public void preConnect() {
				Message m = new Message();
				m.obj = "connecting...";
				TestBtActivity.this.h.sendMessage(m);
			}
			
			@Override
			public void postConnect() {
				Message m = new Message();
				m.obj = "connected!!!";
				TestBtActivity.this.h.sendMessage(m);
			}
			
			@Override
			public void doneSyncCallback() {
				Message m = new Message();
				m.obj = "finished sync";
				TestBtActivity.this.h.sendMessage(m);				
			}
		};
    }
    
    @Override
    protected void onResume() {
    	super.onResume();
    }
    
    @Override
    protected void onPause() {
    	super.onPause();
    	bt.stop();
    }
    
    //avoid orientation change restart the application
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
    	super.onConfigurationChanged(newConfig);
    }
}