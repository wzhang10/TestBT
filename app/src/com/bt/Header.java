package com.bt;

public class Header{
	//ensure packets delivery
	public static final byte signature = 0x75;
	public byte size;
	public short seq;
	public short ack;
	
	//higher protocol
	public byte action;
	public byte unused;
	public short extra;
	
	public static final int headerSize = 10;

	//Action/Flags
	public static final byte BT_SYN 		= (byte) 0x01;
	public static final byte BT_ACK 		= (byte) 0x02;
	public static final byte BT_REQ_KEY 	= (byte) 0x04;
	public static final byte BT_SEND_KEY 	= (byte) 0x08;
	public static final byte BT_NO_KEY	 	= (byte) 0x10;
	public static final byte BT_NO_SUCH_REQ	= (byte) 0x20;
	public static final byte BT_SEND_G		= (byte) 0x40;
	public static final byte BT_LAST	 	= (byte) 0x80;
	private static short currentSeq = 1;
	
	public Header(byte size, short ack, byte action) {
		this.size = size;
		this.seq = getCurrentSeq();
		this.ack = ack;
		this.action = action;
		this.extra = (short)0xffff;
	}
	
	public Header(){
		
	}
	
	public byte[] toByteArray(){
		byte[] b = new byte[headerSize];
		int i=0;
		b[i++]=signature;
		b[i++]=size;
		b[i++]=(byte)(seq & 0xff);
		b[i++]=(byte)((seq >> 8) & 0xff);
		b[i++]=(byte)(ack & 0xff);
		b[i++]=(byte)((ack >> 8) & 0xff);
		b[i++]=action;
		b[i++]=(byte)0xff;
		b[i++]=(byte)(extra & 0xff);
		b[i++]=(byte)((extra >> 8) & 0xff);
		return b;
	}
	
	public static Header getHeader(byte[] data){
		if(data.length<headerSize)return null;
		if(data[0]!=signature)return null;

		Header h = new Header();
		int i=1;
		h.size = data[i++];
		h.seq = (short)(data[i++] & 0xff);
		h.seq += data[i++] << 8;
		h.ack = (short)(data[i++] & 0xff);
		h.ack += data[i++] << 8;
		h.action = data[i++];
		i++;
		h.extra = (short)(data[i++] & 0xff);
		h.extra += data[i++] << 8;
		return h;
	}
	
	public void setExtra(short e){extra = e;}
	public void setSize(byte s){size = s;}
	
	private static short getCurrentSeq(){
		if(currentSeq==-1)
			currentSeq=0;
		return currentSeq++;
	}
}