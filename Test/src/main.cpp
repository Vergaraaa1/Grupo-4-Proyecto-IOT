#include <Arduino.h>
#include <StarterKitNB.h>
#include <SparkFun_SHTC3.h>
#include <ADC121C021.h>
#include <Wire.h>

//Definiciones (variables, funciones, etc)

#define EN_PIN WB_IO6

#define ALERT_PIN WB_IO5

#define ADC121C021_ADDRESS 0x50
#define MQ2_ADDRESS 0x51

#define RatioMQ2CleanAir (1.0) //RS / R0 = 1.0 ppm
#define MQ2_RL (10.0)		   //the board RL = 10KΩ  can adjust



//Las def deben ir arriba... si se hacen en el loop no funcioinara

SHTC3 g_shtc3;
StarterKitNB sk;

//Msg
String temperature = "";
String humidity = "";
String gas = "";
String msg = "{\"" + temperature + "\":" + humidity + "}";

//APN  

String apn = "m2m.entel.cl";
String user = "entelpcs";
String pw = "entelpcs";

//ThingsBoard

String clientId = "grupo4";
String userName = "44444";
String password = "44444";

//fns



//error
void errorDecoder(SHTC3_Status_TypeDef message)   // The errorDecoder function prints "SHTC3_Status_TypeDef" resultsin a human-friendly way
{
  switch (message)
  {
    case SHTC3_Status_Nominal:
      Serial.print("Nominal");
      break;
    case SHTC3_Status_Error:
      Serial.print("Error");
      break;
    case SHTC3_Status_CRC_Fail:
      Serial.print("CRC Fail");
      break;
    default:
      Serial.print("Unknown return code");
      break;
  }
}
//error


//aa
void shtc3_read_data()
{
	float Temperature = 0;
	float Humidity = 0;
	
	SHTC3_Status_TypeDef result = g_shtc3.update();
	if (g_shtc3.lastStatus == SHTC3_Status_Nominal) // You can also assess the status of the last command by checking the ".lastStatus" member of the object
	{

		Temperature = g_shtc3.toDegC();			          // Packing LoRa data
		Humidity = g_shtc3.toPercent();
		
		Serial.print("RH = ");
		Serial.print(g_shtc3.toPercent()); 			      // "toPercent" returns the percent humidity as a floating point number
		Serial.print("% (checksum: ");
		
		if (g_shtc3.passRHcrc) 						            // Like "passIDcrc" this is true when the RH value is valid from the sensor (but not necessarily up-to-date in terms of time)
		{
			Serial.print("pass");
		}
		else
		{
			Serial.print("fail");
		}
		
		Serial.print("), T = ");
		Serial.print(g_shtc3.toDegC()); 			        // "toDegF" and "toDegC" return the temperature as a flaoting point number in deg F and deg C respectively
		Serial.print(" deg C (checksum: ");
		
		if (g_shtc3.passTcrc) 						            // Like "passIDcrc" this is true when the T value is valid from the sensor (but not necessarily up-to-date in terms of time)
		{
			Serial.print("pass");
		}
		else
		{
			Serial.print("fail");
		}
		Serial.println(")");
	}
	else
	{
    Serial.print("Update failed, error: ");
		errorDecoder(g_shtc3.lastStatus);
		Serial.println();
	}
}
//aa

ADC121C021 MQ2;

void setup() {
  sk.Setup(true);

  
  pinMode(ALERT_PIN, INPUT);
	pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH); //power on RAK12004'''

  delay(500);

  time_t timeout = millis();

  Serial.begin(115200);

  while (!Serial)
	{
		if ((millis() - timeout) < 5000)
		{
			delay(100);
		}
		else
		{
			break;
		}
	}


 

  while (!(MQ2.begin(MQ2_ADDRESS, Wire)))
 {
		Serial.println("please check device!!!");
		delay(200);
	}
	Serial.println("RAK12004 test Example");

  Wire.begin();


  //adc.begin();


  sk.UserAPN(apn,user,pw);
  delay(500);
  sk.Connect(apn);
  Serial.print("Beginning sensor. Result = "); 
  errorDecoder(g_shtc3.begin());
  Serial.println("\n\n");

  	MQ2.setRL(MQ2_RL);
	MQ2.setA(-0.890);			//A -> Slope, -0.685
	MQ2.setB(1.125);			//B -> Intersect with X - Axis  1.019
	//Set math model to calculate the PPM concentration and the value of constants
	MQ2.setRegressionMethod(0); //PPM =  pow(10, (log10(ratio)-B)/A)

  float calcR0 = 0;
	for (int i = 1; i <= 100; i++)
	{
		calcR0 += MQ2.calibrateR0(RatioMQ2CleanAir);
	}
	MQ2.setR0(calcR0 / 10);
	if (isinf(calcR0))
	{
		Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply");
		while (1)
			;
	}
	if (calcR0 == 0)
	{
		Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply");
		while (1)
			;
	}
	Serial.printf("R0 Value is:%3.2f\r\n", MQ2.getR0());
}


void loop() {
  if (!sk.ConnectionStatus())
  {
  sk.Reconnect(apn);
  }

  if (!sk.LastMessageStatus)
  {
  sk.ConnectBroker(clientId,userName,password);
  }
  else

 shtc3_read_data();
  //temperature = "";
 // humidity = "";
  //int gasValue = adc.readADC_SingleEnded(ADC121C021::CHANNEL_1);


  	float sensorPPM;
	float PPMpercentage;

  // Serial.println("Getting Conversion Readings from ADC121C021");
	// Serial.println(" ");
   sensorPPM = MQ2.readSensor();

  // Serial.printf("sensor PPM Value is: %3.2f\r\n", sensorPPM);
   PPMpercentage = sensorPPM / 10000;
	// Serial.printf("PPM percentage Value is:%3.2f%%\r\n", PPMpercentage);
  // Serial.println(" ");
	// Serial.println("        ***************************        ");
	// Serial.println("");
  // Serial.println(" ");
	// Serial.println("        ***************************        ");
	// Serial.println(" ");

  float valTemp = g_shtc3.toDegC();
  float valHum = g_shtc3.toPercent();
  



  msg = "{ \"temperature\":"+ String(valTemp) + 
  ", \"humidity\":" + String(valHum)+ ",\"PPM%\":" + String(PPMpercentage)+", \"PPM\":" + String(sensorPPM)+"}"; 



  Serial.println(msg);
  delay(20000);

//   sk.SendMessage(msg);
}
