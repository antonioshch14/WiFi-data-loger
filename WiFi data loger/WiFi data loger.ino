#include <U8g2lib.h>
#include <Wire.h>
#include "DHTesp.h"
#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include "Network.h"
#include "Sys_Variables.h"
#include "CSS.h"
#include <SPI.h>




//===================time===============



class TimeSet {
	int monthDays[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 }; // API starts months from 1, this array starts from 0

	int *shift;
	time_t lastUpdate;//time when the time was updated, used for current time calculation
	time_t actual_Time;
	time_t UNIXtime;
public:
	int NowMonth;
	int NowYear;
	int NowDay;
	int NowWeekDay;
	int lastShift;
	int NowHour;
	int NowMin;
	int NowSec;
	int Now10Min;
	int sec() {
		updateCurrecnt();
		return actual_Time % 60;
	};
	int min() {
		updateCurrecnt();
		return actual_Time / 60 % 60;
	};
	int hour() {
		updateCurrecnt();
		return actual_Time / 3600 % 24;
	};
	
	time_t SetCurrentTime(time_t timeToSet) { //call each time after recieving updated time
		lastUpdate = millis();
		UNIXtime = timeToSet;
	}//call each time after recieving updated time
	void updateDay() {
		updateCurrecnt();
		breakTime(&NowYear, &NowMonth, &NowDay, &NowWeekDay);
		Serial.println(String(NowYear) + ":" + String(NowMonth) + ":" + String(NowDay) + ":" + String(NowWeekDay));
		}// call in setup and each time when new day comes 
	bool Shift() {  //check whether there is a new day comes, first call causes alignement of tracked and checked day 
		//Serial.println("lastShift " + String(lastShift) + "  *shift " + String(*shift));
		if (lastShift != *shift) {
			lastShift = *shift;
			 return true;
		}
		else return false;
	} //call to check whether the day is changed
	void begin(int shiftType) //1-day,2-hiur,3-10 minutes
	{
		switch (shiftType)
		{
		case 1:shift = &NowDay;
			break;
		case 2:shift = &NowHour;
			break;
		case 3:shift = &Now10Min;
			break;
		default:shift = &NowDay;
			break;
		}
	};
private:
	
	void updateCurrecnt() {
		actual_Time= UNIXtime +(millis() - lastUpdate) / 1000;
			
	}
	
	bool LEAP_YEAR(time_t Y) {
		//((1970 + (Y)) > 0) && !((1970 + (Y)) % 4) && (((1970 + (Y)) % 100) || !((1970 + (Y)) % 400)) ;
		if ((1970 + (Y)) > 0) { if ((1970 + (Y)) % 4 == 0) { if ((((1970 + (Y)) % 100) != 0) || (((1970 + (Y)) % 400) == 0)) return true;
			}else return false; }else return false;
		}
	
	void breakTime( int *Year,int *Month,int *day,int *week) {
	// break the given time_t into time components
	// this is a more compact version of the C library localtime function
	// note that year is offset from 1970 !!!

	int year;
	int month, monthLength;
	uint32_t time;
	unsigned long days;

	time = (uint32_t)actual_Time;
	NowSec = time % 60;
	time /= 60; // now it is minutes
	NowMin = time % 60;
	Now10Min = 10*(time % 60);
	time /= 60; // now it is hours
	NowHour = time % 24;
	time /= 24; // now it is days
	*week = int(((time + 4) % 7) );  // Monday is day 1 

	year = 0;
	days = 0;
	while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
		year++;
	}
	*Year = year+1970; // year is offset from 1970 

	days -= LEAP_YEAR(year) ? 366 : 365;
	time -= days; // now it is days in this year, starting at 0

	days = 0;
	month = 0;
	monthLength = 0;
	for (month = 0; month < 12; month++) {
		if (month == 1) { // february
			if (LEAP_YEAR(year)) {
				monthLength = 29;
			}
			else {
				monthLength = 28;
			}
		}
		else {
			monthLength = monthDays[month];
		}

		if (time >= monthLength) {
			time -= monthLength;
		}
		else {
			break;
		}
	}
	*Month = month + 1;  // jan is month 1  
	*day = time + 1;     // day of month
}
};
TimeSet Time_set;
String fileName = "/temp.csv";

DHTesp dht;
float temp;
float humid;
uint8_t sda = 13;//AM2302
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R1, /* reset=*/ U8X8_PIN_NONE);  // Full screen buffer mode

#define ONE_HOUR 3600000UL

ESP8266WebServer server(80);             // create a web server on port 80

File fsUploadFile;                                    // a File variable to temporarily store the received file

//ESP8266WiFiMulti wifiMulti;    // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

const char *OTAName = "ESP8266";         // A name and a password for the OTA service
const char *OTAPassword = "esp8266";

const char* mdnsName = "esp8266";        // Domain name for the mDNS responder

WiFiUDP UDP;                   // Create an instance of the WiFiUDP class to send and receive UDP messages

IPAddress timeServerIP;        // The time.nist.gov NTP server's IP address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48;          // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE];      // A buffer to hold incoming and outgoing packets
bool NTPconnected = false;
uint32_t actualTime;
class task {
public:
	unsigned long period;
	bool ignor = false;
	void reLoop() {
		taskLoop = millis();
	};
	bool check() {
		if (!ignor) {
			if (millis() - taskLoop > period) {
				taskLoop = millis();
				return true;
			}
		}
		return false;
	}
	void StartLoop(unsigned long shift) {
		taskLoop = millis() + shift;
	}
	task(unsigned long t) {
		period = t;
	}
private:
	unsigned long taskLoop;

};
task am2302udate(2000);//read sensor
task displayUpdate(1000);
task task1(10000);//connection to NTP
task taskSendNTPrequest(100);//sending NTP request
task writeToLog(10000);
class fileSt {
public:
	String name;
	String size;
	bool checked;//true if the file checked on the number
	unsigned long number_csv=0;
	void checkNameOnNumber(String beginer, String end) {
	int fieldBegin = name.indexOf(beginer)+ beginer.length();
	int check_field = name.indexOf(beginer);
	int filedEnd = name.indexOf(end);
	number_csv = 0;
	if (check_field != -1 && filedEnd != -1) {
		for (int i = filedEnd-1; i > fieldBegin; i--) {
			char ch = name[i];
			if (isDigit(ch)) {
				int val = ch - 48;
				number_csv += ((val * pow(10, filedEnd - 1-i)));
			}
			if (number_csv > 999999999)break;
		}
		checked = false;
	}
	else checked = true;
	}
	};

int filesStoredIndex;
const int maxFiles = 10;
fileSt filesStored[maxFiles];


void setup() {
	Serial.begin(9600);
	u8g2.begin();
	u8g2.setFont(u8g2_font_crox1c_tf);
	Wire.begin();
	pinMode(sda, INPUT);
	dht.setup(sda, DHTesp::AM2302);

	startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection

	startOTA();                  // Start the OTA service

	startSPIFFS();               // Start the SPIFFS and list all contents

	startMDNS();                 // Start the mDNS responder
	Time_set.begin(2);               //start time 1-day,2-hour,3-10 min
//startServer();               // Start a HTTP server with a file read handler and an upload handler

startUDP();                  // Start listening for UDP messages to port 123
server.on("/", HomePage);
server.on("/download", File_Download);
server.on("/Delete", File_Delete);
server.begin();


	/*WiFi.hostByName(ntpServerName, timeServerIP); // Get the IP address of the NTP server
	Serial.print("Time server IP:\t");
	Serial.println(timeServerIP);

	sendNTPpacket(timeServerIP);
	delay(500);*/

}
const unsigned long intervalNTP = ONE_HOUR; // Update the time every hour
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();

const unsigned long intervalTemp = 60000;   // Do a temperature measurement every minute
unsigned long prevTemp = 0;
bool tmpRequested = false;
const unsigned long DS_delay = 750;         // Reading the temperature from the DS18x20 can take up to 750ms

uint32_t timeUNIX = 0;                      // The most recent timestamp received from the time server


void showOnOled(String t, String v) {
	u8g2.clearBuffer();
	u8g2.setCursor(0, 10);
	u8g2.print(t);
	u8g2.setCursor(0, 21);
	u8g2.print(v);
	 u8g2.drawStr(0, 32, "3");
	u8g2.drawStr(0, 43, "4");
	 u8g2.drawStr(0, 54, "5");
	u8g2.setCursor(0, 65);
	u8g2.print("6");
	u8g2.setCursor(0, 76);
	u8g2.print("7");
	u8g2.drawStr(0, 87, "8");
	u8g2.drawStr(0, 98, "9");
	u8g2.drawStr(0, 109, "10");
	u8g2.drawStr(0, 120, "11");
	u8g2.sendBuffer();
}
bool sensorRead(float*t, float*h) {
	*h = dht.getHumidity();
	*t = dht.getTemperature();
	if (isnan(*t) || isnan(*h)) {
		Serial.println("Failed to read from DHT22 sensor! ");
		return false;
	}
	return true;
}
bool connectToNtp() {
	if (WiFi.hostByName("time.nist.gov", timeServerIP)) { // Get the IP address of the NTP server

		Serial.print("Time server IP:\t");
		Serial.println(timeServerIP);

		Serial.println("\r\nSending NTP request ...");
		sendNTPpacket(timeServerIP);
		return true;
	}
	Serial.println("DNS lookup failed.");
	startUDP();
	return false;
}

void loop() {
	if (am2302udate.check()) {
		if (sensorRead(&temp, &humid)) {}
		else { temp = 0; humid = 0; };
	}
	if(displayUpdate.check())showOnOled(String(temp), String(humid));
	yield();

	

	if (task1.check()) {
		Serial.println("Check()");
		if (WiFi.status() == WL_CONNECTED && !NTPconnected) {
			Serial.println("WL_CONNECTED");
			NTPconnected = connectToNtp();
			task1.ignor = NTPconnected;
		}
		else {
			Serial.println("fail to connect to WiFI");
			WiFi.disconnect();
			startWiFi();
		}
	}
	if (NTPconnected) {
		unsigned long currentMillis = millis();

		if (taskSendNTPrequest.check()) { // If a minute has passed since last NTP request
			//prevNTP = currentMillis;
			Serial.println("\r\nSending NTP request ...");
			sendNTPpacket(timeServerIP);               // Send an NTP request
			taskSendNTPrequest.period = 100;//hurry up to obtain time from NTP
		}
		yield();
		uint32_t time = getTime();                   // Check if an NTP response has arrived and get the (UNIX) time
		if (time) {                                  // If a new timestamp has been received
			Serial.println("time " + String(time));
			timeUNIX = time;
			Serial.print("NTP response:\t");
			Serial.println(timeUNIX);
			lastNTPResponse = currentMillis;
			taskSendNTPrequest.period = 1000 * 60 * 60 * 24;//slow down requestes when the time is obtained
			Time_set.SetCurrentTime(timeUNIX);
		}

		actualTime = timeUNIX + (currentMillis - lastNTPResponse) / 1000;
		
		if (timeUNIX&&writeToLog.check()) {
			Time_set.updateDay();
			Serial.println("/" + String(Time_set.NowYear) + String(Time_set.NowMonth) +
				String(Time_set.NowDay) + String(Time_set.NowHour) + String(Time_set.NowMin)+ String(Time_set.NowSec));
			String Time = String(Time_set.NowHour) + ":" + String(Time_set.NowMin) + ":" + String(Time_set.NowSec);
			
			
			Serial.println(temp+String("  ")+humid);
			String Fmonth, Fday,Fhour, Fmin;
			if (Time_set.Shift()){	
			if (Time_set.NowMonth < 10)Fmonth ="0"+ String(Time_set.NowMonth);
			else Fmonth = String(Time_set.NowMonth);
			if (Time_set.NowDay < 10)Fday = "0" + String(Time_set.NowDay);
			else Fday = String(Time_set.NowDay);
			if (Time_set.NowHour < 10)Fhour = "0" + String(Time_set.NowHour);
			else Fhour = String(Time_set.NowHour);
			if (Time_set.NowMin < 10)Fmin = "0" + String(Time_set.NowMin);
			else Fmin = String(Time_set.NowMin);
					fileName = "/" + String(Time_set.NowYear) + Fmonth+ Fday+ Fhour+Fmin +".csv";
				Serial.println("Appending temperature to file: " + fileName);
				File tempLog = SPIFFS.open(fileName, "a"); // Write the time and the temperature to the csv file
				tempLog.print(Time);
				tempLog.print(',');
				tempLog.print("Temp");
				tempLog.print(',');
				tempLog.println("Humid");
				tempLog.close();
				startSPIFFS();
			}
			File tempLog = SPIFFS.open(fileName, "a"); // Write the time and the temperature to the csv file
			tempLog.print(Time);
			tempLog.print(',');
			tempLog.print(temp);
			tempLog.print(',');
			tempLog.println(humid);
			tempLog.close();
			
		}
	}


	
	yield();
	server.handleClient(); // run the server 
	yield();
	ArduinoOTA.handle();
}
void startWiFi() { // Try to connect to some given access points. Then wait for a connection

	char ssid[] = "Keenetic-4574"; //  Your network SSID (name)
	char pass[] = "Gfmsd45kaxu69$"; // Your network password
	WiFi.begin(ssid, pass);
	WiFiClient client; // Initialize the WiFi client library
	Serial.println("WiFi connected");
}

void startUDP() {
	Serial.println("Starting UDP");
	UDP.begin(123);                          // Start listening for UDP messages to port 123
	Serial.print("Local port:\t");
	Serial.println(UDP.localPort());
}

void startOTA() { // Start the OTA service
	ArduinoOTA.setHostname(OTAName);
	ArduinoOTA.setPassword(OTAPassword);

	ArduinoOTA.onStart([]() {
		Serial.println("Start");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\r\nEnd");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
	Serial.println("OTA ready\r\n");
}
String findOldest() {
		unsigned long min = 9999999999;
		int indexOfMin = 0;
		bool found;
		for (int i = 0; i <= maxFiles-1; i++) {
			Serial.printf("min %d <= filesStored[i].number_csv %d && !filesStored[i].checked %b",
				min, filesStored[i].number_csv); Serial.println(filesStored[i].checked);
			if (!filesStored[i].number_csv && min >= filesStored[i].number_csv && !filesStored[i].checked) {
				indexOfMin = i;
				min = filesStored[i].number_csv;
				found = true;
			}
		}
		
		Serial.printf(" File name %s position %d\n\r", filesStored[indexOfMin].name.c_str(), indexOfMin);
		if(found) return filesStored[indexOfMin].name;
		else return "Not_found";
}; 
void startSPIFFS() { // Start the SPIFFS and list all contents
	SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
	filesStoredIndex = -1;
	Serial.println("SPIFFS started. Contents:");
	{   
		Dir dir = SPIFFS.openDir("/");
		while (dir.next()) {                      // List the file system contents
			String fileName = dir.fileName();
			size_t fileSize = dir.fileSize();
			Serial.printf("\tFS File: %s, size: %s  \r\n", fileName.c_str(), formatBytes(fileSize).c_str()); //\r\n
			if (filesStoredIndex <= maxFiles) {
				filesStoredIndex++;
				filesStored[filesStoredIndex].name = fileName;
				filesStored[filesStoredIndex].size = formatBytes(fileSize);
				filesStored[filesStoredIndex].checkNameOnNumber("/",".csv");
				Serial.print(filesStored[filesStoredIndex].number_csv);
				//Serial.printf(" filesStoredIndex %d \r\n", filesStoredIndex);
				//deleteFile(fileName);
				
				if (filesStoredIndex >= maxFiles ) {
					for (int i = 0; i < maxFiles - filesStoredIndex; i++) {
						Serial.printf(" maxFiles - filesStoredIndex = %d, i= %d \r\n", maxFiles - filesStoredIndex, i);
						if (deleteFile(findOldest()))   refreshSPIFS();
					}
				}
			}
		}
		Serial.printf("\n");
	}
	
}

void startMDNS() { // Start the mDNS responder
	MDNS.begin(mdnsName);                        // start the multicast domain name server
	Serial.print("mDNS responder started: http://");
	Serial.print(mdnsName);
	Serial.println(".local");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
	server.on("/edit.html", HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
		server.send(200, "text/plain", "");
	}, handleFileUpload);                       // go to 'handleFileUpload'

	server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
	// and check if the file exists

	//server.begin();                             // start the HTTP server
	Serial.println("HTTP server started.");
}

/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
	if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
		server.send(404, "text/plain", "404: File Not Found");
	}
}
bool handleFileRead(String path) { // send the right file to the client (if it exists)
	Serial.println("handleFileRead: " + path);
	if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
	String contentType = getContentType(path);             // Get the MIME type
	String pathWithGz = path + ".gz";
	if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
		if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
			path += ".gz";                                         // Use the compressed verion
		File file = SPIFFS.open(path, "r");                    // Open the file
		size_t sent = server.streamFile(file, contentType);    // Send it to the client
		
		file.close();                                          // Close the file again
		Serial.println(String("\tSent file: ") + path);
		return true;
	}
	Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
	return false;
}
void handleFileUpload() { // upload a new file to the SPIFFS
	HTTPUpload& upload = server.upload();
	String path;
	if (upload.status == UPLOAD_FILE_START) {
		path = upload.filename;
		if (!path.startsWith("/")) path = "/" + path;
		if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
			String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
			if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
				SPIFFS.remove(pathWithGz);
		}
		Serial.print("handleFileUpload Name: "); Serial.println(path);
		fsUploadFile = SPIFFS.open(path, "w");               // Open the file for writing in SPIFFS (create if it doesn't exist)
		path = String();
	}
	else if (upload.status == UPLOAD_FILE_WRITE) {
		if (fsUploadFile)
			fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
	}
	else if (upload.status == UPLOAD_FILE_END) {
		if (fsUploadFile) {                                   // If the file was successfully created
			fsUploadFile.close();                               // Close the file again
			Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
			server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
			server.send(303);
		}
		else {
			server.send(500, "text/plain", "500: couldn't create file");
		}
	}
}
/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/
String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
	if (bytes < 1024) {
		return String(bytes) + "B";
	}
	else if (bytes < (1024 * 1024)) {
		return String(bytes / 1024.0) + "KB";
	}
	else if (bytes < (1024 * 1024 * 1024)) {
		return String(bytes / 1024.0 / 1024.0) + "MB";
	}
	return "";
}
String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
	if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}
unsigned long getTime() { // Check if the time server has responded, if so, get the UNIX time, otherwise, return 0
	if (UDP.parsePacket() == 0) { // If there's no response (yet)
		return 0;
	}
	UDP.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
	// Combine the 4 timestamp bytes into one 32-bit number
	uint32_t NTPTime = (packetBuffer[40] << 24) | (packetBuffer[41] << 16) | (packetBuffer[42] << 8) | packetBuffer[43];
	// Convert NTP time to a UNIX timestamp:
	// Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
	const uint32_t seventyYears = 2208988800UL;
	// subtract seventy years:
	int32_t UNIXTime = NTPTime - seventyYears + 60 * 60 * 3;
	
	return UNIXTime;
}
void sendNTPpacket(IPAddress& address) {
	Serial.println("Sending NTP request");
	memset(packetBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
	// Initialize values needed to form NTP request
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode

	// send a packet requesting a timestamp:
	UDP.beginPacket(address, 123); // NTP requests are to port 123
	UDP.write(packetBuffer, NTP_PACKET_SIZE);
	UDP.endPacket();
}

void HomePage() {
	Serial.println("Home page begins");
	
  SendHTML_Header();
  webpage += F("<a href='/download'><button>Download</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header() {
	//server.send(200, "text/plain", "Home page begins"); 
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
 server.setContentLength(CONTENT_LENGTH_UNKNOWN);
 // server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  append_page_header();
  SendHTML_Content();
  webpage = "";
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop() {
  server.sendContent("");
  server.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ReportFileNotPresent(String target) {
	SendHTML_Header();
	webpage += F("<h3>File does not exist</h3>");
	webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
	append_page_footer();
	SendHTML_Content();
	SendHTML_Stop();
}
//===========================================================================================

  void File_Download() { // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
	  if (server.args() > 0) { // Arguments were received
		  if (server.hasArg("download")) SD_file_download(server.arg(0));
		  
	  }
	  else SelectInput("File Download", "Enter filename to download", "download", "download");
  }
  void File_Delete() { // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
	  if (server.args() > 0) { // Arguments were received
		  if (server.hasArg("Delete")) if (deleteFile(server.arg(0))) {
			}
		   server.sendHeader("Connection", "close");
	  }
	  else SelectInput("File Delete", "Enter filename to delete",  "Delete", "Delete");
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void SD_file_download(String filename) {
	  File download = SPIFFS.open("/" + filename, "r");
	  if (download) {
		  server.sendHeader("Content-Type", "text/text");
		  server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
		  server.sendHeader("Connection", "close");
		  server.streamFile(download, "application/octet-stream");
		  download.close();
	  }
	  else ReportFileNotPresent("download");
  }
  bool deleteFile(String filename) {
	  bool filefound = false;
	  Serial.println("Tring to delete file  .... " + filename);
	  //File download = SPIFFS.open("/" + filename, "r");
	  File download = SPIFFS.open( filename, "r");
	  if (download) filefound = true;
	  download.close();
	  if (filefound) {
		  //SPIFFS.remove("/"+filename);
		  SPIFFS.remove( filename);
		  Serial.println("File deleted "+ filename);
		
		  return true;
	  }
	  return false;
  }
  void refreshSPIFS() {
	  for (int i = 0; i <= filesStoredIndex; i++) {
		  filesStored[i].name = "";
		  filesStored[i].size = "";
		  filesStored[i].checked = false;
		  filesStored[i].number_csv = 0;
		  }
	//  filesStoredIndex = 0;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void SendHTML_Content() {
	  Serial.println("webpage: " + String(webpage.length()));
	  server.sendContent(webpage);
	  webpage = "";
  }
 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void SelectInput(String heading1, String heading2, String command, String arg_calling_name) {
	  startSPIFFS();
	  SendHTML_Header();
	  webpage += F("<h3 class='rcorners_m'>"); webpage += heading1 + "</h3><br>";
	  webpage += F("<h3>"); webpage += heading2 + "</h3>";
	  webpage += F("<h4>"); webpage += filesStored[0].name+ filesStored[0].size + "</h4>";
	  webpage += F("<h5>"); webpage += filesStored[1].name + filesStored[1].size  + "</h5>";
	  webpage += F("<h6>"); webpage += filesStored[2].name + filesStored[2].size + "</h6>";
	  webpage += F("<h7>"); webpage += filesStored[3].name + filesStored[3].size + "</h7>";
	  webpage += F("<h8>"); webpage += filesStored[4].name + filesStored[4].size + "</h8>";
	  webpage += F("<h9>"); webpage += filesStored[5].name + filesStored[5].size + "</h9>";
	  webpage += F("<h10>"); webpage += filesStored[6].name + filesStored[6].size + "</h10>";
	  webpage += F("<h11>"); webpage += filesStored[7].name + filesStored[7].size + "</h11>";
	  webpage += F("<h12>"); webpage += filesStored[8].name + filesStored[8].size + "</h12>";
	  webpage += F("<h13>"); webpage += filesStored[9].name + filesStored[9].size + "</h13>";
	  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
	  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
	  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
	  
	  append_page_footer();
	  SendHTML_Content();
	  SendHTML_Stop();
  }
  