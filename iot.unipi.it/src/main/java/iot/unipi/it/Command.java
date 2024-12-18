package iot.unipi.it;

import com.fasterxml.jackson.annotation.JsonProperty;

public class Command {

	@JsonProperty("name")
	public String name;
	
	@JsonProperty("value")
	public String value;
		
	public Command()
	{}
	
	public Command(String name, String value)
	{
		this.name = name;
		this.value = value;
	}
	
}