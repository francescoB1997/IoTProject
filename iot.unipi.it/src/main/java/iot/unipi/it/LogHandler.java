package iot.unipi.it;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;

import java.util.List;
import java.io.File;
import java.io.IOException;
import java.sql.Timestamp;
import java.text.SimpleDateFormat;
import java.util.ArrayList;

public class LogHandler 
{
	private String filePath; // "log_09:00.json"
	private String folderPath; // "./Log/Log_2022-08-05/"
	private File F;
	
	public LogHandler()
	{
		this.folderPath = Constants.DEFAULT_LOG_PATH + "/Log_" + new SimpleDateFormat("yyyy-MM-dd").format(new Timestamp(System.currentTimeMillis()));
		this.filePath = this.folderPath + "/log_" + new SimpleDateFormat("HH").format(new Timestamp(System.currentTimeMillis())) + ".json";
		
		this.F = new File(Constants.DEFAULT_LOG_PATH);
		if(!this.F.canRead())
			this.F.mkdir();
	}
	
	public LogHandler(String day)
	{
		//System.out.println("LogHandler(String day)");
		this.folderPath = Constants.DEFAULT_LOG_PATH + "/Log_" + day;
		this.filePath = "";
		
		this.F = new File(Constants.DEFAULT_LOG_PATH);
		if(!this.F.canRead())
			this.F.mkdir();
	}
	
	public List<Log> getLogListFromJsonFile(String path)
	{
		List<Log> tempLogList;
		ObjectMapper mapper = new ObjectMapper();
		mapper.configure(SerializationFeature.INDENT_OUTPUT, true);
		try
		{
			tempLogList = mapper.readValue(new File(path), new TypeReference<List<Log>>() {});
		}
		catch(Exception e)
		{
			tempLogList = new ArrayList<Log>();
		}
		return tempLogList;
	}
		
	public void WriteLog(String event)
	{
		try 
		{	
			this.folderPath = Constants.DEFAULT_LOG_PATH + "/Log_" + new SimpleDateFormat("yyyy-MM-dd").format(new Timestamp(System.currentTimeMillis()));
			this.F = new File(this.folderPath);
			if(!this.F.canRead())
				this.F.mkdir();
			
			this.filePath = this.folderPath + "/log_" + new SimpleDateFormat("HH").format(new Timestamp(System.currentTimeMillis())) + ".json";
			this.F = new File(this.filePath);
			
			ObjectMapper mapper = new ObjectMapper();
			mapper.configure(SerializationFeature.INDENT_OUTPUT, true);

			mapper.setDateFormat(new SimpleDateFormat("yyyy-MM-dd HH:mm:ss"));
			
			List<Log> templogList = this.getLogListFromJsonFile(this.filePath);
			templogList.add(new Log(event));
			JsonNode newRoot = mapper.valueToTree(templogList);
			mapper.writeValue(this.F, newRoot);

        }
		catch (Exception e) 
		{
			if(e.getClass() == JsonProcessingException.class)
				System.out.println("JsonProcessingException");
			else if(e.getClass() == IOException.class)
				System.out.println("IOException");
			else
				System.out.println("Esle exception--> " + e.getClass());
        }
    }
	
	public void printLogListByDay(String day)
	{
		List<Log> tempLogList;
		String tempPath;
		File tempFile;
		tempPath = "./" + Constants.DEFAULT_LOG_PATH + "/Log_" + day;
		if(! new File(tempPath).canRead())
		{
			System.out.println("There is no log for " + day);
		}
		try
		{
			for(int i = 0; i < 24; i++)
			{
				if(i < 10)
					tempPath = "./" + Constants.DEFAULT_LOG_PATH + "/Log_" + day + "/log_0" + i + ".json";
				else
					tempPath = "./" + Constants.DEFAULT_LOG_PATH + "/Log_" + day + "/log_" + i + ".json";	
				tempFile = new File(tempPath);
							
				if(!tempFile.canRead())
					continue;
				
				tempLogList = this.getLogListFromJsonFile(tempPath);
				
				System.out.println("\n\u001B[31m **************************** LOG OF " + day + " HOUR " + i + " ****************************");
				for(Log logInList : tempLogList)
					logInList.printInfo();
				System.out.println("\u001B[31m *********************************************************************************** \u001B[0m \n");
			}
		}
		catch(Exception e)
		{
			System.out.println("Catch getLogByDay");
		}
	}	
}
