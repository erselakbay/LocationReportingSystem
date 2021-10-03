#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Wire.h>

TinyGPS gps;

SoftwareSerial ss(10,11);                                          //Gps(port1) for tx rx location information
SoftwareSerial sim800(3,2);                                        //Gsm(port2) for tx rx location information

                                  
String Data_SMS;                                                   //SMS that we will send to the phone number

int darbesensor = 9;                                               //Vibration sensor required for impact
int acc = 6;                                                       //Ignition (ACC)

uint8_t lastsat = 0;                                               //If the GPS connection cannot be found in shake situations, to memorize the last position to discard the last position.
float lastflat = 0;
float lastflon = 0;

long vibration = 0;                                                //For vibration status and vibration level.
int vibrationstatus = 0;

int accstatus = 0;                                                 //For ACC status and vibration level.
int accCounter = 1;

int alt2, speed2, year2;                                           //I made a global definition with the name 2 so that the data from the GPS can be found from anywhere in the code.
byte month2, day2, hour2, minute2, second2;



void setup()
{
  Serial.begin(9600);                                              //I start communication with the computer
  pinMode(darbesensor,INPUT);                                      //I state that the sensors are inputs.
  pinMode(acc,INPUT);
  ss.begin(9600);                                                  //Initializing the first software communication port
  sim800.begin(9600);                                              //Initializing the second software communication port
  delay(10000);                                                    //Delay to allow the module to connect to the network. Removable
}

void loop()
{

  vibrationstatus=0;                                        
  ss.listen();                                                      //Gps listening
  smartdelay(1000); 
  Serial.println();
  vibration = pulseIn(darbesensor,HIGH);
  Serial.print("Value read from vibration sensor: ");               //I print the value from the vibration sensor to the screen (for control purposes)
  Serial.println(vibration);
  accstatus=digitalRead(acc);                                       //To get ACC information whenever we want
  
  if ( accstatus == HIGH && accCounter == 1)                        //To send sms when acc is opened
  {
     Serial.println("Acc Open");
     sendlocation();
     accCounter = 0;
  }
  
  if( accstatus == LOW)                                             //Just to send sms when the ACC is opened. As long as the ACC is open, it is prevented 
                                                                    //from constantly entering "if" in the loop and sending continuous sms.
  {
    accCounter = 1;
  }
  
  if (vibration > 10000 && accstatus == LOW)                        //To send sms if vibration occurs when acc is off
  {
     vibrationstatus = 1;
     Serial.println("TOUCHED (0)");
     Serial.println("Last Location: ");
     Serial.print("Number of satellites: ");
     Serial.println(lastsat);
     Serial.print("Latitude: ");
     Serial.println(lastflat,6);
     Serial.print("Longitude: ");
     Serial.println(lastflon,6);
     sendlocation();
     vibration = 0;
     delay(25000);                                                   //Delay to understand whether the vibration is continuous or instantaneous(Also sms saving)
  }

    Serial.print(" No TOUCHED (1)");

}

//Functions


static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}



int sendlocation()                                                 //Writes to the screen without finding the exact location, but does not send sms. I always printed it on the screen 
                                                                   //for control purposes.
                                                                   //When it finds a location, it memorizes the number of satellites and coordinates (for times when no gps signal is found),
                                                                   //then sends all the information via SMS.
{   
  int looper = 1;
  int counter=0; 
  
  while (looper==1)                                                //I put it in a loop to scan until the it finds gps. Loop will be broken when gps signal is found.            
  {

    Serial.println("Location Searching");

    uint8_t sat = gps.satellites();
    Serial.print("Number of satellites: ");
    Serial.println(sat);


    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat,&flon,&age);
    Serial.print("Latitude: ");
    Serial.println(flat,6);
    Serial.print("Longitude: ");
    Serial.println(flon,6);


    int alt = gps.f_altitude();
    Serial.print("Altitude: ");
    Serial.println(alt);
    alt2=alt;                                                      //I am sending to global variables

    int speed = gps.f_speed_kmph();
    Serial.print("Speed: ");
    Serial.println(speed);
    speed2=speed;                                                  //I am sending to global variables

    int crs = gps.f_course();
    Serial.print("Direction(Angle): ");
    Serial.println(crs);

    int year;
    byte month, day, hour, minute, second, hundredths;
    unsigned long age2;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age2);


    Serial.print("Date: ");
    Serial.print(day);
    Serial.print(".");
    Serial.print(month);
    Serial.print(".");
    Serial.println(year);
    day2=day;                                                       //I am sending to global variables
    month2=month;
    year2=year;

    Serial.print("Hour : ");
    Serial.print(hour+3);                                           //Add or subtract according to your country's time difference.
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.println(second);
    hour2=hour+3;                                                   //I am sending to global variables
    minute2=minute;
    second2=second;

    delay(1000);


    if(sat < 15 || counter > 15)                                    //Send as SMS if gps result is not found in 15 attempts (location in memory) OR new gps signal is found (new location)
    {
      lastsat = sat;                                                //Refresh the location data in memory when new location
      lastflat = flat;
      lastflon = flon;
      sim800.listen();                                              //We need to Gsm. Gsm listening
      Send_Data();
      looper=0;
      counter=0;
    }
    counter++;                                                      //For 15 gps search attempts
  }
}




void Send_Data()
{
  sim800.print("AT+CMGF=1\r");                                      //Set the module to SMS mode
  delay(100);
  sim800.print("AT+CMGS=\"+905351234567\"\r");                      //Your phone number don't forget to include your country code example +90535xxxxxxx"
  delay(500);
  Data_SMS =  "Hey! A dangerous situation has been detected! " ;                                         //This string is sent as SMS
  delay(500);
  
  sim800.print(Data_SMS + day2 + "."  + month2 + "." + year2 + " Hour: " 
  + hour2 + ":" + minute2 + ":" + second2 + ""                                                           //Optional sms content
  + " Acc:" + accstatus + " Shock:" + vibrationstatus + " Speed:" + speed2 + " " 
  + "https://www.google.com/maps/place/" + String(lastflat,6)+ "," + String(lastflon,6));
  
  delay(500);
  sim800.print((char)26);                                           //Required to tell the module that it can send the SMS
  delay(500);
  sim800.println();
  delay(500);
  Serial.println("Sms Sent");
}
