package iot.unipi.it;

public class Constants 
{
	public static final String clientID = "JAVA Subscriber";
	public static final String brokerIP = "tcp://127.0.0.1:1883";
	public static final String SubTopic = "viaNapoli/data";
	
	public static final String PubTopic = "viaNapoli/commands" ;
	
	public static final int minThreshold = 10;
	public static final int maxThreshold = 100;
	
	public static final String DEFAULT_LOG_PATH = "Logs";
}
