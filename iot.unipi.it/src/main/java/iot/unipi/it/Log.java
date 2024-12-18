package iot.unipi.it;

import java.sql.Timestamp;
import java.text.SimpleDateFormat;

public class Log 
{
	public String time;
	public String event;
	
	public Log()
	{
		this.time = "";
		this.event = "";
	}
	
	public Log(String event)
	{
		this.time = new SimpleDateFormat("dd-MM-yyyy HH:mm:ss").format(new Timestamp(System.currentTimeMillis()));
		this.event = event;
	}
	
	public void printInfo()
	{
		System.out.println("\u001B[32m | TIME: " + this.time + " | EVENT: " + this.event + " |");
	}

}