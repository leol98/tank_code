/**
  Preliminary Code for running the tank
  Serial is USB interface
  Serial1 is GPS
  Serial2 is to Tank Controller board
  Serial3 is XBee
  t
  STAHP normally works but might not, mind your feet
 */

#include <Wire.h>
#include <LSM303.h>
#include <TinyGPS.h>
#include <stdlib.h>

#define XBeebaud 9600
#define TankController 9600

#define margin 3
#define NUM_POINTS 2 /*default 2, can be manully input later */

LSM303 compass;

#define GPSbaud 9600
TinyGPS gps;
unsigned long cur=0;
struct GPSdata{
	float GPSLat=0;
	float GPSLon=0;
	unsigned long GPSTime=0;
	long GPSAlt=0;
	int GPSSats=-1;
};

typedef struct point{
	float lat;
	float lon;
	char *name;
	int arr;
} Point;
GPSdata gpsInfo;
GPSdata preserve;
bool newData;
bool stream;

//--- Active Variables
Point startpoint;
Point destinationloc;
Point pos;//my pos
//---

Point **destinations;

int logicState = 1;//1:init   2:recCmD   3:nav  4:Return/end

int pointnum = 0;//Number of points being navigated between

int counter=0;

void motor_control(int, int);

Point* create_point(float lon, float lat, char *name);

void setup() {
	int i = 0;
	Serial.begin(9600);
	Serial1.begin(GPSbaud);
	Serial3.begin(XBeebaud);
	Serial2.begin(9600);
	Wire.begin();
	compass.init();
	compass.enableDefault();
	compass.m_min = (LSM303::vector<int16_t>){-32767, -32767, -32767};
	compass.m_max = (LSM303::vector<int16_t>){+32767, +32767, +32767};
	Serial.println("Init:-");


	destinations = calloc(NUM_POINTS + 1, sizeof(Point*));

	destinations[0] = create_point(-76.942629, 38.988075, "hornbake");
	destinations[1] = create_point(-76.942828, 38.988053, "home");
	destinations[2] = NULL;
}

void loop() {  
	//Sense Time
	delayMicroseconds(500);  
	preserve = gpsInfo;
	while (Serial1.available()){
		if (gps.encode(Serial1.read())){
			newData = true;
			gpsInfo = getGPS();
			pos.lat = gpsInfo.GPSLat;
			pos.lon = gpsInfo.GPSLon;
			break;
		}
	}

	switch(logicState){
		case 1:{//Init			
					 if(millis()>=(cur+1000)){
						 cur=millis();
						 Serial.println(gpsInfo.GPSSats);
						 if(gpsInfo.GPSSats>0){//Waits for 
							 Serial.println("GPS Lock");
							 Serial3.println("GPS Lock");
							 Serial.println("My Location: "+String(gpsInfo.GPSLat,6) + " , " + String(gpsInfo.GPSLon,6));
							 logicState = 2;	
						 }else{
							 Serial.println("GPSerr");
							 Serial3.println("GPSerr");
						 }
					 }
				 }break;
		case 2:{//receive Commands // Defaulted to Hornbake center for now
					 Serial3.println("to3");
					 runCommand();
					 logicState = 3;
					 Serial.println("to3");
				 }break;
		case 3:{//Navigate
					 if(gpsInfo.GPSSats<0){
						 Serial.println("GPSLCKERR");
					 }else{
						 if(millis()>=(cur+1000)){
							 cur=millis();
							 if(stream){
								 Serial.println((String(gpsInfo.GPSLat,6) + "," + String(gpsInfo.GPSLon,6))+" , ");
								 Serial3.println((String(gpsInfo.GPSLat,6) + "," + String(gpsInfo.GPSLon,6))+" , ");
								 Serial3.println(getCompass());
								 Serial.println(getCompass());
							 }
						 }
					 }
					 runCommand();
					 if(nav(destinations[pointnum])){
						if(!destinations[++pointnum]){
							logicState = 5;
						}else{
							logicState = 4;
						}
					 }
				 }break;
		case 4:{
					 stop();
					 Serial3.print("Arrived to location%d", pointnum-1);
					 /*do whatever need to do at that point */
					 delay(1000);
					 break;
				 }
		case 5: { Serial3.print("MISSION DONE");
					  stop();
					  Serial3.println((String)(millis()/1000));
					  logicState = 6;
				  }break;
		case 6: delay(100);
				  runCommand();
				  free_point();/*should go somewhere else*/
				  break;
	}
}

int nav(Point *dest){
	float distance, direction;
	turntopoint(dest, &distance, &direction);
	if((distance < margin)&&(distance>0)){
		stop();
		return 1;
	}
	if(distance >= margin){
		if(distance>20){
			forward(9);
		}else if(distance>10){
			forward(5);
		}else{
			forward(3);
		}
	}
	return 0;
}

GPSdata getGPS(){
	GPSdata gpsInfo;
	newData = false;
	unsigned long chars;
	unsigned short sentences, failed;

	// For one second we parse GPS data and report some key values
	unsigned long start = millis();

	float GPSLat, GPSLon;
	int GPSSats;
	long GPSAlt;
	unsigned long date,fix_age,GPSTime;
	gps.f_get_position(&GPSLat, &GPSLon, &fix_age);
	GPSSats = gps.satellites();
	gps.get_datetime(&date, &GPSTime, &fix_age);
	GPSAlt = gps.altitude()/100.;

	gpsInfo.GPSLat = GPSLat;
	gpsInfo.GPSLon = GPSLon;
	gpsInfo.GPSTime = GPSTime;
	gpsInfo.GPSSats = GPSSats;
	gpsInfo.GPSAlt = GPSAlt;

	return gpsInfo;
}

void runCommand(){
	int uplink = 0;
	String message;
	//UPLINK (GND-->Tank)
	//Intake commands to be interpreted (might not work, TEST)
	if(Serial3.available()>0){//Lets you send commands direct via USB
		uplink = Serial3.read();
		Serial.print("Serial Command: ");
	}else if(Serial.available()>0){//Over Xbee
		Serial.print("Xbee Command: ");
		uplink = Serial.read();
	}
	if(Serial2.available()>0){//Over Xbee
		Serial.print("Error Message");
		Serial3.print("Error Message");
		String mes = Serial2.readString();
		//delay(50);
		Serial3.println(mes);
		Serial.println(mes);
	}

	//---
	//The switch (predefined passthroughs n stuff)
	switch(uplink){
		case	't': Serial.println("TEST");
					  Serial3.println("TEST");
					  delay(1000);break;

		case	'p':	message = (String(gpsInfo.GPSLat,6) + "," + String(gpsInfo.GPSLon,6));
						Serial3.println(message);
						Serial.println("P command");
						Serial.println(message);
						break;

		case	'r': rth();
					  Serial.println("RTH");
					  Serial3.println("RTH");
					  break;//return to home

		case	'q': sethome();
					  Serial.println("HomeSet");
					  Serial3.println("HomeSet");
					  break;//return to home

		case	's' : stop();
						delay(50);
						stop();
						Serial.println("STAHP");
						Serial3.println("STAHP");
						logicState = 6;
						break;

		case	'n' : Serial.println("Restart");
						Serial3.println("Restart");
						logicState = 1;
						delay(500);break;

		case	'g' : Serial.println("Toggle");//Toggle GPS stream output
						Serial3.println("Toggle");
						if(stream){
							stream = 0;
						}else{
							stream = 1;
						}
						delay(500);break;						

		default	: break;
	}
}

///---Nav Methods---///
void rth(){
	destinationloc = startpoint;
}

void sethome(){
	startpoint = pos;
}

///---Control Methods---///
void forward(int power){
	Serial2.write('F');
	Serial2.write(power+'0');
}

void backward(int power){
	Serial2.write('B');
	Serial2.write(power+'0');
}

void stop(){
	Serial2.write('S');
}

void right(int power){
	Serial2.write('R');
	Serial2.write(power+'0');
}

void left(int power){
	Serial2.write('L');
	Serial2.write(power+'0');
}

void turntopoint(Point *target, float *distance, float *direction){//heading-current    direction-desired
	bearing(pos.lat,pos.lon,target->lat, target->lon, distance, direction);
	float change = *direction - getCompass();
	if((abs(change)>5)&&(abs(change)<355)){
		if((change>0)&&(change<=180)){
			if(change<=30){
				right(1);
			}else if(change<90){
				right(2);
			}else{
				right(6);
			}
		}else if(change>180){
			if(change<195){
				left(6);
			}else if(change<270){
				left(4);
			}else{
				left(2);
			}
		}else if((change<0)&&(change>=-180)){
			if(change>-30){
				left(1);
			}else if(change>-90){
				left(2);
			}else{
				left(6);
			}
		}else if(change<-180){
			right(1);//I don't remember what this guy does
		}
		change = *direction - getCompass();
	}
}

float getCompass(){
	compass.read();
	float t1 = compass.heading();
	compass.read();
	float t2 = compass.heading();
	compass.read();
	float t3 = compass.heading();
	compass.read();
	float t4 = compass.heading();
	float heading = (t1+t2+t3+t4)/4.0;
	return heading;
}

Point* create_point(float lon, float lat, char *name){
	Point *tmp = calloc(1, sizeof(Point));
	tmp->lat = lat;
	tmp->lon = lon;
	tmp->arr = 0;
	tmp->name = calloc(strlen(name) + 1, 1);
	strcpy(tmp->name, name);
	return tmp;
}

void free_point(){
	int i = 0;
	while(destinations[i]){
		free(destinations[i]->name);
		free(destinations[i]);
		i++;
	}
	free(destinations);
}
