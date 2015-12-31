/* 
 * Database functions
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include "database.h"
#include "logger.h"
#include <libpq-fe.h>
#include "common.h"
#include <string.h>

PGconn *psql;

// inserts collected values into the database
int insertIntoDatabase(struct weather* w)
{
    // initialise the result pointer
    PGresult *dbResult;

    // Query String buffer
    char queryString[BUFSIZ];
	
    // Date string buffer
    char dateString[60];
    
    // Get the date string
    getTime(dateString, sizeof(dateString));
	
    // this is the query string, writes all variables to the string
    snprintf(queryString, sizeof(queryString), "INSERT INTO %s (\"time\", \"in_humidity\", \"out_humidity\", \"in_temperature\"," 
			  "\"out_temperature\", \"out_dew_temperature\", \"windchill_temperature\", \"wind_speed\"" 
			  ", \"wind_gust\", \"wind_direction\", \"pressure\", \"rel_pressure\", \"rain_last_hour\"" 
			  ", \"rain_last_24h\", \"rain_total\",\"bytes\") values (now(), '%d', '%d', '%0.1f', %0.1f" \
			  ", '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%s', '%0.1f', '%0.1f', '%0.1f','%0.1f', '%0.1f', '%s');",
			  dbase.table, w->in_humidity, w->out_humidity, w->in_temp,
			  w->out_temp, w->dew_point, w->wind_chill, w->wind_speed, w->wind_gust,
			  w->wind_dir, w->abs_pressure, w->rel_pressure, w->last_hour_rain_fall, 
			  w->last_24_hr_rain_fall, w->total_rain_fall, w->readBytes);

    // If debug turned on, show the connection string
    if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "insertIntoDatabase", "Query String:: %s", queryString);	 			 

    // execute the connection string
    dbResult = PQexec(psql, queryString);

    // If the command was successful
    if( PQresultStatus(dbResult) == PGRES_COMMAND_OK )
    {
      // If debug turned on, announce success
      if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "insertIntoDatabase", "Entered into Database ok", NULL);
    }
    else
    {
      // command failed, write error to error output
      if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "insertIntoDatabase", "Command Failed:: %s", PQerrorMessage(psql));
    }      
}

// makes the database connection
int connectToDatabase()
{
  // This is the buffer going to the server
  char connectionInfo[BUFSIZ];
  // This is the buffer going to screen.. changes the password, so its not displayed on screen
  char debugConnectionInfo[BUFSIZ];
  
  sprintf(connectionInfo, "host=%s dbname=%s port=%s user=%s password=%s", dbase.host, dbase.dbname,
	  dbase.port, dbase.user, dbase.password);

  sprintf(debugConnectionInfo, "host=%s dbname=%s port=%s user=%s password=%s", dbase.host, dbase.dbname,
	  dbase.port, dbase.user, "*****");
 
  // writes the connection string information to debug
  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "connectToDatabase", "Sending connection string:: %s", debugConnectionInfo);
  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "connectToDatabase", "Attempting to connect to database server", NULL);
  
  /* init connection */
  psql = PQconnectdb(connectionInfo);
  
  // Connection failed
  if (PQstatus(psql) != CONNECTION_OK) {
      // Connection error, put the error to error output
      if(log_sort.all || log_sort.database) {
		logger(LOG_DEBUG, logType, "connectToDatabase", "libpq error: PQstatus(psql) != CONNECTION_OK", NULL);
		logger(LOG_DEBUG, logType, "connectToDatabase", "PSQL Error Message: '%s'", PQerrorMessage(psql));
      }
	  exit(0);
  } else {
	  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "connectToDatabase", "Connection made to the database", NULL);
	  // connection to the database was good, write to debug output
	  return 1;
  }
}

void openConfigFile ()
{  
    // line buffer size
    char line[BUFSIZ];
  
    // Hard code the file name for the config file
    char fileName[] = "config";
     
    FILE *configFile;   
    
    int  i;
    
    char *position;

    char _buffer[BUFSIZ];
            
     // Open the configuration file up for reading
    configFile = fopen(fileName, "r");

    // If the config file didn't open
    if(!configFile)
    {
      // Spit an error 
      if(log_sort.all || log_sort.database) logger( LOG_ERROR, logType, "OpenConfigFile", "The config file not found", NULL );
      
      // exit the program
      exit(0);
    } 
   
    if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "OpenConfigFile", "Config file opened", fileName);

    // Get a line from the configuration file
    while( fgets(line, sizeof(line), configFile) != NULL ) 
    { 	
      // enable if you want  to see output
	// logger(LOG_DEBUG, logType, "Checking for parameters line #", rtrim(line));
	
      // Detect if there is a comment present
		if(line[0] != '#')
		{
			// Look for the delimiter that denotes where the value is
			position = strchr(line, '=');               
						
			// initialise i
			i = 0;                       
			
			// while not ; (end of line)
			while(line[(++position)-line] != ';')
			{
			  // put the value from the line
			  _buffer[i++] = line[position - line];	
			}    
			
			// Copy the value to the database array
			if(strstr(line, "host"))
			{
			  strncpy(dbase.host, _buffer, i);
			  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "Found Host ", dbase.host);
			}
			if(strstr(line, "dbname"))
			{
			  strncpy(dbase.dbname, _buffer, i);
			  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "Found dbname ", dbase.dbname);
			}
			if(strstr(line, "table"))
			{
			  strncpy(dbase.table, _buffer, i);
			  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "Found table ", dbase.table);
			}	      
			if(strstr(line, "port"))
			{
			  strncpy(dbase.port, _buffer, i);
			  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "Found port ", dbase.port);
			}
			if(strstr(line, "dbuser"))
			{
			  strncpy(dbase.user, _buffer, i);
			  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "Found user ", dbase.user);
			}	     
			if(strstr(line, "password"))
			{
			  strncpy(dbase.password, _buffer, i);
			  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "Found password ", "*****", NULL);
			}	    
			if(strstr(line, "wgUserName"))
			{
			  strncpy(dbase.wgUserName, _buffer, i);
			  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "Found wgUserName ", dbase.wgUserName);
			}	    			
			if(strstr(line, "wgPassword"))
			{
			  strncpy(dbase.wgPassword, _buffer, i);
			  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "Found wgPassword ", "******", NULL);
			}	    				
			if(strstr(line, "wgId"))
			{
			   strncpy(dbase.wgId, _buffer, i);
   			   if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "Found wgId ", dbase.wgId);
			}
		}
    }    
    
    if(log_sort.all || log_sort.database) logger( LOG_DEBUG, logType, "OpenConfigFile", "Closing the Config file", NULL );
    // Close the file
    fclose(configFile);
}
