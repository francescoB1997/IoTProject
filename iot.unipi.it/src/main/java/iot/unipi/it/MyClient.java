package iot.unipi.it;

import java.io.*;
import java.sql.Timestamp;
import java.text.SimpleDateFormat;

import org.eclipse.californium.core.*;
import org.eclipse.californium.core.coap.MediaTypeRegistry;
import org.eclipse.paho.client.mqttv3.*;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;

public class MyClient implements MqttCallback 
{
	private static MqttClient myMqttClient;	
	private DBHandler db;
	public static LogHandler logHandler;
	static public CoapClient client;
	public boolean irrigAutoMode;	// | true(default) ==> AUTOMATIC | false ==> MANUAL |
	
	public MyClient() throws MqttException
	{
		db = new DBHandler();
		irrigAutoMode = true;
		logHandler = new LogHandler();
		try
		{ 
			client = new CoapClient("coap://[fd00::f6ce:36ed:babb:5620]:5683/pumpState");
		}
		catch(Exception e)
		{
			System.out.println("I can't find the CoAP Server");
			client = null;
		}
		myMqttClient = new MqttClient(Constants.brokerIP, Constants.clientID);
		myMqttClient.setCallback(this);
		myMqttClient.connect();
		try {
		myMqttClient.subscribe(Constants.SubTopic);
		}
		catch(Exception e)
		{
			System.out.println("Eccezione subscrive mqtt--> " + e.getMessage());
		}
		System.out.println("Subscription completed");
	}
	
	public void connectionLost(java.lang.Throwable cause)
	{
		System.out.println("connectionLost");
	}
	
	public void messageArrived(java.lang.String topic, MqttMessage message) throws Exception
	{
		String payload =  new String(message.getPayload());
		System.out.println("Arrivato da mqtt--> " + payload);
		this.db.addEventToDB(topic , payload);
		
		if(topic.contentEquals(Constants.SubTopic) && payload.contains("pumpState")) //This command arrives from MqttNode
		{
			client.put("payload=" + payload, MediaTypeRegistry.TEXT_PLAIN);
		}
		else if(topic.contentEquals(Constants.SubTopic) && payload.contains("moisture")) //moisture
		{
			ObjectMapper mapper = new ObjectMapper();
			Command C = mapper.readValue(payload, Command.class);
			MyClient.logHandler.WriteLog( C.name + "=" + C.value);
		}
		else if(topic.contentEquals(Constants.SubTopic) && payload.contains("irrigMode")) // Tasto premuto sul dongle
		{
			Command C;
			
			this.irrigAutoMode = (payload.contains("AUTO")) ? true : false;
			
			C = new Command("irrigMode", (this.irrigAutoMode) ? "AUTO" : "MAN");
			
			System.out.println("New irrigation mode--> " + C.value);
			MyClient.logHandler.WriteLog( C.name + "=" + C.value);
		}
	}

	public void deliveryComplete(IMqttDeliveryToken token)
	{
		//System.out.println("deliveryComplete");
	}
	
	public void publish(String topic, String message) throws MqttException
	{
		MqttMessage mqttMessage = new MqttMessage(message.getBytes());
		myMqttClient.publish(topic, mqttMessage);
	}
	
	public static void main(String[] args)
	{
		boolean exitChoice = false;
		int drySoilThreshold = 25, wetSoilThreshold = 60;
		
		final MyClient myClient;
		try
		{
			myClient = new MyClient();
		}
		catch (Exception e)
		{
			if(e.getClass() == MqttException.class)
				System.out.println("Problem with MQTT");
			else
				System.out.println("General Problem");
			return;
		}
		if(client != null)
		{
			CoapObserveRelation relation = client.observe(
					 new CoapHandler() {
						 public void onLoad(CoapResponse response) 
						 {
							String payload = response.getResponseText().toString();
							//System.out.println("COAP NOTIFICATION --> " + payload);
							try
							{
								if(payload.contains("pumpState")) // Se viene modificato lo stato della risorsa, viene informato anche il MoistureNode(MqttNode)
								{
									if(payload.contains("ON"))
									{
										MyClient.logHandler.WriteLog("pumpState: ON");
										//System.out.println("The pump is ON");
									}
									else if(payload.contains("OFF"))
									{
										MyClient.logHandler.WriteLog("pumpState: OFF");
										//System.out.println("The pump is OFF");
									}
									else
									{
										System.out.println("******************* Pump error");
										return;
									}
									System.out.println("Ricevuto da coapServer---> " + payload);
									myClient.publish(Constants.PubTopic, payload);
								}
							}
							catch(Exception e)
							{
								System.out.println("EXCEPTION CoAP onLoad--> " + e.getMessage());
							}
						}
						
						 public void onError() {
							System.err.println("OBSERVING FAILED (press enter to exit)");
						}
					});
		}
		
		BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
		System.out.println("\n *************************** IrrigApp ****************************");
		MyClient.logHandler.WriteLog("Starting IrrigApp");
		while(!exitChoice)
		{
				System.out.println("\u001B[33m\n Possible commands:");
				System.out.println("  1: Start irrigation (if MAN mode is enabled)");
				System.out.println("  2: Stop irrigation (if MAN mode is enabled");
				System.out.println("  3: Change dry-soil threshold ");
				System.out.println("  4: Change wet-soil threshold ");
				System.out.println("  5: Show the today's log (splitted by Hour)");
				System.out.println("  6: Show the another day's log (splitted by Hour)");
				System.out.println("  9: Close app"); //Bianco -->  \u001B[0m
				
				int choice;
				try
				{
					choice = Integer.parseInt(br.readLine());
				}
				catch(Exception e)
				{
					continue;
				}
				Command C;
				ObjectMapper mapper = new ObjectMapper();
				mapper.configure(SerializationFeature.INDENT_OUTPUT, false);
				switch(choice)
				{
					case 1:
						if(myClient.irrigAutoMode)
						{
							System.out.println("Press first the button on Moisture Node to change the irrigation mode.");
							continue;
						}
						
						C = new Command("pumpState", "ON");
						try
						{
							String JsonCommand = mapper.writeValueAsString(C);
							client.put("payload=" + JsonCommand, MediaTypeRegistry.TEXT_PLAIN);
						}
						catch(Exception e)
						{
							System.out.println("Eccezione---> " + e.getMessage());
						}
						break;
					case 2:
						if(myClient.irrigAutoMode)
						{
							System.out.println("Press first the button on Moisture Node to change the irrigation mode.");
							continue;
						}
						C = new Command("pumpState", "OFF");
						try
						{
							String JsonCommand = mapper.writeValueAsString(C);
							//myClient.publish(Constants.PubTopic, JsonCommand); //myClient.publish(Constants.PubTopics[1], "OFF");
							client.put("payload=" + JsonCommand, MediaTypeRegistry.TEXT_PLAIN);
						}
						catch(Exception e)
						{
							System.out.println("Eccezione---> " + e.getMessage());
						}
						
						break;
					case 3:
						System.out.print("Insert new value for dry-soil threshold [ " + Constants.minThreshold + " - " + Constants.maxThreshold + " ]: ");
						int newDrySoilValue = -1;
						try
						{
							newDrySoilValue = Integer.parseInt(br.readLine());
						}
						catch(Exception e)
						{
							System.out.println("Illegal value!");
							continue;
						}
						
						if( (newDrySoilValue < Constants.minThreshold) || (newDrySoilValue > Constants.maxThreshold) || (newDrySoilValue > wetSoilThreshold))
						{
							System.out.println("Illegal value!");
							continue;
						}
						C = new Command("newThresholdMin", String.valueOf(newDrySoilValue));
						try
						{
							String JsonCommand = mapper.writeValueAsString(C);
							
							myClient.publish(Constants.PubTopic, JsonCommand); //myClient.publish(Constants.PubTopics[2], String.valueOf(newDrySoilValue));
						}
						catch(Exception e)
						{
							System.out.println("Eccezione---> " + e.getMessage());
						}
						//System.out.println("newDrySoilValue --> " + newDrySoilValue);
						MyClient.logHandler.WriteLog( C.name + "=" + C.value);
						drySoilThreshold = newDrySoilValue;
						break;
					case 4:
						System.out.print("Insert new value for wet-soil threshold [ " + Constants.minThreshold + " - " + Constants.maxThreshold + " ]: ");
						int newWetSoilValue = -1;
						try {
							newWetSoilValue = Integer.parseInt(br.readLine());
						}
						catch(Exception e)
						{
							System.out.println("Illegal value!");
							continue;
						}
						
						
						if( (newWetSoilValue < Constants.minThreshold) || (newWetSoilValue > Constants.maxThreshold) || (newWetSoilValue < drySoilThreshold))	
						{
							System.out.println("Illegal value!");
							continue;
						}
						C = new Command("newThresholdMax", String.valueOf(newWetSoilValue));
						try
						{
							String JsonCommand = mapper.writeValueAsString(C);
							System.out.println("Risultato-->" + JsonCommand);
							myClient.publish(Constants.PubTopic, JsonCommand); //myClient.publish(Constants.PubTopics[3], String.valueOf(newWetSoilValue));
						}
						catch(Exception e)
						{
							System.out.println("Eccezione---> " + e.getMessage());
						}
						//System.out.println("newWetSoilValue--> " + newWetSoilValue);
						MyClient.logHandler.WriteLog( C.name + "=" + C.value);
						wetSoilThreshold = newWetSoilValue;
						break;
					case 5:
						LogHandler tempLog = new LogHandler();
						tempLog.printLogListByDay(new String (new SimpleDateFormat("YYYY-MM-dd").format(new Timestamp(System.currentTimeMillis()))));
						break;
					case 6:
						LogHandler tempLog_ = new LogHandler();
						System.out.print(" Insert the desired date (please, respect the format YYYY-MM-DD): ");
						String userDate  = "";
						try 
						{
							userDate = br.readLine();
						}
						catch(Exception e)
						{
							
						}
						if(!userDate.isEmpty())
							tempLog_.printLogListByDay(userDate);
						break;
					case 9:
						MyClient.logHandler.WriteLog("Closing IrrigApp");
						exitChoice = true;
						break;
					default:
						//System.out.println("DEFAULT--> " + choice);
						break;
				};
		}
		
		
	System.exit(0);
	}
	
}