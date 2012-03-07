package com.bt;

import android.util.Log;

//just a wrapper class
public abstract class BluetoothKM {
	MainThread mt;
	
	public BluetoothKM() {
	}
	
	public boolean sync(Object data){
		if(mt!=null && mt.isAlive())return false;
		mt = new MainThread() {
			@Override
			public void nomoreKey() {
				BluetoothKM.this.nomoreKey();
			}			
			@Override
			public void gotKey(byte[] key, int index) {
				BluetoothKM.this.gotKey(key, index);
			}			
			@Override
			public void preConnect() {
				BluetoothKM.this.preConnect();				
			}			
			@Override
			public void postConnect() {
				BluetoothKM.this.postConnect();
			}
			@Override
			public void doneSendKeys() {
				BluetoothKM.this.doneSyncCallback();
			}
		};
		mt.prepareAction(Action.SYNC, data);
		mt.start();
		return true;
	}
	
	public boolean requestKey(){
		if(mt!=null && mt.isAlive())return false;
		mt = new MainThread() {
			@Override
			public void nomoreKey() {
				BluetoothKM.this.nomoreKey();
			}			
			@Override
			public void gotKey(byte[] key, int index) {
				BluetoothKM.this.gotKey(key, index);
			}			
			@Override
			public void preConnect() {
				BluetoothKM.this.preConnect();				
			}			
			@Override
			public void postConnect() {
				BluetoothKM.this.postConnect();
			}
		};
		mt.prepareAction(Action.REQ_KEY, null);
		mt.start();
		return true;
	}
	
	public abstract void nomoreKey();
	public abstract void gotKey(byte[] key, int index);
	public void postConnect() {}
	public void preConnect() {}
	public void doneSyncCallback(){}
	public void ConnectionTimeoutCallback(){}
	public void stop(){
		if(mt != null && mt.isAlive()){
			Log.e(LogcatTag.TAG, "killing thread");
			mt.abort();
		}
	}
		
	public void turnOnBluetooth(){
		MainThread.turnOnBluetooth();
	}
	
	public void turnOnBluetoothBlocking(int timeoutms){
		MainThread.turnOnBluetoothBlocking(timeoutms);
	}
}
