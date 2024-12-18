package iot.unipi.it;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.Timestamp;
import java.text.SimpleDateFormat;

public class DBHandler 
{
	private final String connectionString;
	private final String DBName;
	private final String DBUser = "root";
	private final String DBEventTable;
	private static Connection connectionToDB;

	public DBHandler()
	{
		DBName = "db_iot";
		DBEventTable = "event_";
		connectionString = "jdbc:mysql://localhost:3306/" + this.DBName;
	}
	
	public void addEventToDB(String eventDescription, String value)
	{
		final PreparedStatement InsertEventQueryStatement;
		try
		{
			Class.forName("com.mysql.jdbc.Driver").newInstance();
			connectionToDB = DriverManager.getConnection(this.connectionString, this.DBUser , "Root123#");
			InsertEventQueryStatement = connectionToDB.prepareStatement("INSERT INTO " + this.DBEventTable + " VALUES ( ?, ?, ?);");  
			InsertEventQueryStatement.setInt(1, 0);
			InsertEventQueryStatement.setString(2, new String (new SimpleDateFormat("yyyy/MM/dd HH:mm:ss").format(new Timestamp(System.currentTimeMillis()))));
			InsertEventQueryStatement.setString(3, eventDescription + ": " + value);
            InsertEventQueryStatement.executeUpdate();
            connectionToDB.close();
		}
		catch (Exception e)
	    {
    	   System.out.println("**** " + e.getClass() + " --> " + e.getMessage());
		}
		//System.out.println("Adding DB --> " + eventDescription + " " + value);
	}
}







