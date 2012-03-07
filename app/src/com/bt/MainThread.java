package com.bt;


import java.io.IOException;
import java.util.Set;
import java.util.UUID;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.os.Looper;
import android.util.Log;

enum Action{
	REQ_KEY,
	SYNC
};

public abstract class MainThread extends Thread{
	
	class ConnectionLostException extends Exception {
		private static final long serialVersionUID = 7422862446246046772L;

		public ConnectionLostException(Exception e) {
			super(e);
		}

		public ConnectionLostException() {
			super("Connection lost");
		}
	}
	
	enum FinishAction{
		GOT_KEY,
		NO_MORE_KEY,
		DONE_SEND_KEYS
	};
	private FinishAction finishAction;
	private Action action;
	private boolean abort;
	private String addr;
	private BluetoothSocket btsocket;
	private boolean turnBTOffWhenDone;
	private final int BUF_SIZE = 1024;
	private byte[] buffer = new byte[BUF_SIZE];
	private Object prepareObj;
	byte[] key;
	int keyIndex;
	int attempts = 5;
	
	private void init(){
		abort = false;
		addr = new String();
		btsocket = null;
		turnBTOffWhenDone = false;		
	}
	
	/* Actions */
	public abstract void gotKey(byte[] key, int index);
	public abstract void nomoreKey();
	public void doneSendKeys(){};
	public void preConnect(){}
	public void postConnect(){}
	
	public void prepareAction(Action a, Object prepareObj){
		action = a;
		this.prepareObj = prepareObj;
	}
	
	private void requestKey(){
		Header h = new Header((byte)4, (short)0, (byte)(Header.BT_REQ_KEY|Header.BT_SYN));
		byte[] data = new byte[Header.headerSize];
		System.arraycopy(h.toByteArray(), 0, data, 0, Header.headerSize);
		try {
			btsocket.getOutputStream().write(data);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	private void sendKeys(byte[] data, boolean last){
		byte[] tmp;
		Header h;
		if(last)
			h = new Header((byte)0, (short)0, 
				(byte)(Header.BT_SYN | Header.BT_SEND_KEY | Header.BT_LAST));
		else
			h = new Header((byte)0, (short)0, 
				(byte)(Header.BT_SYN | Header.BT_SEND_KEY));
	
		if(data==null){
			tmp = new byte[Header.headerSize];
			
		}else{
			tmp = new byte[Header.headerSize+data.length];
			h.setExtra((short)data.length);
			h.setSize((byte)data.length);
		}		
		System.arraycopy(h.toByteArray(), 0, tmp, 0, Header.headerSize);
		if(data!=null)
			System.arraycopy(data, 0, tmp, Header.headerSize, data.length);
		try {
			btsocket.getOutputStream().write(tmp);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	private void sync(){
		byte[][] keys = (byte[][]) prepareObj;
		if(keys==null)return;
		sendKeys(null, false);
		int i=0;
		for(;i<keys.length;i++){
			sendKeys(keys[i], (i==keys.length-1)?true:false);
		}
		try {
			btsocket.getInputStream().read(buffer, 0, BUF_SIZE);
		} catch (IOException e) {}
		finishAction = FinishAction.DONE_SEND_KEYS;
	}
	
	public synchronized void abort(){
		Log.e(LogcatTag.TAG,"arborting thread");
		abort = true;
		if(turnBTOffWhenDone){
			Log.d(LogcatTag.TAG,"turning off bluetooth");
			final BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
			adapter.disable();
		}
		try {
			if(btsocket!=null)
				btsocket.close();
		} catch (IOException e) {
		}
	}
		
	private void receiveKey(){
		try {
			btsocket.getInputStream().read(buffer, 0, BUF_SIZE);
			Header h = Header.getHeader(buffer);
			if(h==null)return;
			
			if((h.action & Header.BT_SEND_KEY)>0){
				key = new byte[h.size];
				System.arraycopy(buffer, Header.headerSize, key, 0, h.size);
				Log.i(LogcatTag.TAG, "key="+new String(key));
				keyIndex = h.extra;
				finishAction = FinishAction.GOT_KEY;
			}
			
			if((h.action & Header.BT_NO_KEY)>0){
				finishAction = FinishAction.NO_MORE_KEY;
			}			
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	public static void turnOnBluetooth(){
		final BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
		adapter.enable();
	}
	
	//input 0 or negative to wait forever
	public static boolean turnOnBluetoothBlocking(int timeoutms){
		final BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
		try {
			adapter.enable();
			while(!adapter.isEnabled()){
				sleep(1);
				timeoutms--;
				if(timeoutms<0)
					timeoutms=0;
				else if(timeoutms==0)
					return false;
			}
			return true;
		} catch (InterruptedException e){
			return false;
		}
	}
	
	private boolean waitForConnect() throws ConnectionLostException, InterruptedException, IOException{
		try {
    		final BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
    		if(!adapter.isEnabled()){
    			if(!turnOnBluetoothBlocking(0))
    				return false;
    		}
    		Log.d(LogcatTag.TAG, "Bluetooth is on");

    		Set<BluetoothDevice> bondedDevices = adapter.getBondedDevices();
    		if(bondedDevices.isEmpty())return false;
    		while(true){
    			for (BluetoothDevice device : bondedDevices) {
    				if (device.getName().startsWith("IOIO")) {
    					addr = device.getAddress();
    					Log.d(LogcatTag.TAG, "found ioio device" + device.getName() + " " + addr);
    					final BluetoothDevice ioioDevice = adapter.getRemoteDevice(addr);
    					synchronized (this) {
    						try {
    							Log.d(LogcatTag.TAG, "Creating socket");
    							btsocket = ioioDevice.createInsecureRfcommSocketToServiceRecord(UUID
    									.fromString("00001101-0000-1000-8000-00805F9B34FB"));
    						} catch (IOException e) {
    							throw new ConnectionLostException();
    						}
    					}
						Log.d(LogcatTag.TAG, "connecting...");
						adapter.cancelDiscovery();
						btsocket.connect();
						Log.d(LogcatTag.TAG, "connected to "+ addr);
						return true;
    				}
    			}
    		}
    	} catch (SecurityException e) {
    		Log.e(LogcatTag.TAG, "no permission?");
    		throw e;
    	}
	}
	

	@Override
	public void run() {
		super.run();
		init();
		Looper.prepare();
		Log.d(LogcatTag.TAG, "thread started");
		if(abort)return;
		while(true){//attempts--!=0){
			try {
				preConnect();
				if(waitForConnect()){
					postConnect();
					Log.d(LogcatTag.TAG, "finished waiting for connection");
					if(action==Action.SYNC){
						Log.d(LogcatTag.TAG, "sync-ing");
						sync();
					}else if(action==Action.REQ_KEY){
						Log.d(LogcatTag.TAG, "REQ_KEY");
						requestKey();
						receiveKey();
					}
					Log.d(LogcatTag.TAG, "closing socket");
		    		btsocket.close();
		    		
		    		switch(finishAction){
		    		case GOT_KEY:
		    			gotKey(key, keyIndex);
		    			return;
		    		case NO_MORE_KEY:
		    			nomoreKey();
		    			return;
		    		case DONE_SEND_KEYS:
		    			doneSendKeys();
		    			return;
		    		}
				}
			} catch (ConnectionLostException e) {
				Log.e(LogcatTag.TAG, "connection lost exeception");
				if(abort)return;
			} catch (InterruptedException e) {
				Log.e(LogcatTag.TAG, "Interrupted Exception");
				if(abort)return;
			} catch (Exception e) {
				Log.e(LogcatTag.TAG, e.getLocalizedMessage());
				if(abort)return;
			}
		}
//		Log.e(LogcatTag.TAG, "Connection failed");
	}
}