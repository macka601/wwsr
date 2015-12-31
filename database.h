#ifndef DATABASE_H
#define DATABASE_H

struct databaseAccessValues {
	char user[BUFSIZ];
	char dbname[BUFSIZ];
	char table[BUFSIZ];      
	char port[BUFSIZ];
	char host[BUFSIZ];
	char password[BUFSIZ];  
	char wgUserName[BUFSIZ];
	char wgPassword[BUFSIZ];
	char wgId[BUFSIZ];
} dbase;

extern struct weather weather;

int insertIntoDatabase(struct weather*);
int connectToDatabase();
void openConfigFile();

extern int logType;
#endif /* End of header guard */