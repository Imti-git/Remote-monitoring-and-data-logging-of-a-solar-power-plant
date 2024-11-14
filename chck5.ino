
  #include <SoftwareSerial.h>
SoftwareSerial GSM(2, 4); // RX, TX



enum _parseState {
  PS_DETECT_MSG_TYPE,

  PS_IGNORING_COMMAND_ECHO,

  PS_HTTPACTION_TYPE,
  PS_HTTPACTION_RESULT,
  PS_HTTPACTION_LENGTH,

  PS_HTTPREAD_LENGTH,
  PS_HTTPREAD_CONTENT
};

enum _actionState {
  AS_IDLE,
  AS_WAITING_FOR_RESPONSE
};

byte actionState = AS_IDLE;
unsigned long lastActionTime = 0;

byte parseState = PS_DETECT_MSG_TYPE;
char buffer[200];
byte pos = 0;


float a,b;
char a1[6],b1[5],url[200],c1[10]="007";

#define rst 8

float voltage = 0;
float current = 0;
int senV;int senI;
int samv;float voltagei;float sami;float sen1,i,volti,volt,sen2;float avolt;


int contentLength = 0;

void resetBuffer() {
  memset(buffer, 0, sizeof(buffer));
  pos = 0;
}

void sendGSM(const char* msg, int waitMs = 500) {
  GSM.println(msg);
  while(GSM.available()) {
    parseATText(GSM.read());
  }
  delay(waitMs);
}

void setup()
{
  digitalWrite(rst, HIGH);
  pinMode(rst, OUTPUT);
  
  GSM.begin(9600);
  Serial.begin(9600);

// sendGSM("AT+SAPBR=3,1,\"APN\",\"INTERNET\"");
  sendGSM("AT+SAPBR=3,1,\"APN\",\"gpinternet\"");
  sendGSM("AT+SAPBR=1,1",3000);
  sendGSM("AT+HTTPINIT");  
  sendGSM("AT+HTTPPARA=\"CID\",1");

  //delay(2e3);
}

void loop()
{ 

  senV= senI=0;
  volti=0;avolt=volt=0;
  sen2=sen1=0;
 
 for(int x=0;x<10000;x++){
  
   senI = analogRead(A5); //raw current
   sen1 = sen1+senI;} 

   volti=(sen1/10000)*5/1023;
  
   i=(volti-2.48)/0.1;
   if(i<0) i=0;
   
 for(int x=0;x<10;x++){

  senV = analogRead(A0); //raw voltage
  sen2 = sen2+senV;
   }

   avolt=(sen2/10);
   volt=(avolt*30)/1023;

  a=volt ; //avg voltage
  b= i; //avg current

  //delay(1e3);
   
  unsigned long now = millis();
    
  if ( actionState == AS_IDLE ) {
    //if ( now > lastActionTime + 3000 ) {
      dtostrf( a,4,2,a1); 
      dtostrf( b,3,2,b1);
      Serial.print("Voltage= ");
      Serial.print(a1);
      Serial.print('\n');
      Serial.print("Current= ");
      Serial.print(b1);
      Serial.print('\n');  
      Serial.print('\n');
      sprintf(url,"AT+HTTPPARA=\"URL\",\"http://mocdl.net/api/data/create?voltage=%s&current=%s&motor_id=%s\"",a1,b1,c1);
      //sendGSM(url);
      // set http param value
      Serial.println(url);
      sendGSM(url);
      sendGSM("AT+HTTPACTION=0");
      lastActionTime = now;
      actionState = AS_WAITING_FOR_RESPONSE;
     
   // }
 }

if(actionState == AS_WAITING_FOR_RESPONSE && now > lastActionTime + 4000){

    //Serial.println("reset");
    digitalWrite(rst, LOW);
    
    }
  
  while(GSM.available()) {
    lastActionTime = now;
    parseATText(GSM.read());
  }
}

void parseATText(byte b) {

  buffer[pos++] = b;

  if ( pos >= sizeof(buffer) )
    resetBuffer(); // just to be safe

  /*
   // Detailed debugging
   Serial.println();
   Serial.print("state = ");
   Serial.println(state);
   Serial.print("b = ");
   Serial.println(b);
   Serial.print("pos = ");
   Serial.println(pos);
   Serial.print("buffer = ");
   Serial.println(buffer);*/

  switch (parseState) {
  case PS_DETECT_MSG_TYPE: 
    {
      if ( b == '\n' )
        resetBuffer();
      else {        
        if ( pos == 3 && strcmp(buffer, "AT+") == 0 ) {
          parseState = PS_IGNORING_COMMAND_ECHO;
        }
        else if ( b == ':' ) {
          //Serial.print("Checking message type: ");
          //Serial.println(buffer);

          if ( strcmp(buffer, "+HTTPACTION:") == 0 ) {
            Serial.println("Received HTTPACTION");
            parseState = PS_HTTPACTION_TYPE;
          }
          else if ( strcmp(buffer, "+HTTPREAD:") == 0 ) {
            Serial.println("Received HTTPREAD");            
            parseState = PS_HTTPREAD_LENGTH;
          }
          resetBuffer();
        }
      }
    }
    break;

  case PS_IGNORING_COMMAND_ECHO:
    {
      if ( b == '\n' ) {
        Serial.print("Ignoring echo: ");
        Serial.println(buffer);
        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPACTION_TYPE:
    {
      if ( b == ',' ) {
        Serial.print("HTTPACTION type is ");
        Serial.println(buffer);
        parseState = PS_HTTPACTION_RESULT;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPACTION_RESULT:
    {
      if ( b == ',' ) {
        Serial.print("HTTPACTION result is ");
        Serial.println(buffer);
        parseState = PS_HTTPACTION_LENGTH;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPACTION_LENGTH:
    {
      if ( b == '\n' ) {
        Serial.print("HTTPACTION length is ");
        Serial.println(buffer);
        
        // now request content
        GSM.print("AT+HTTPREAD=0,");
        GSM.println(buffer);
        
        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPREAD_LENGTH:
    {
      if ( b == '\n' ) {
        contentLength = atoi(buffer);
        Serial.print("HTTPREAD length is ");
        Serial.println(contentLength);
        
        Serial.print("HTTPREAD content: ");
        
        parseState = PS_HTTPREAD_CONTENT;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPREAD_CONTENT:
    {
      // for this demo I'm just showing the content bytes in the serial monitor
      Serial.write(b);
      
      contentLength--;
      
      if ( contentLength <= 0 ) {

        // all content bytes have now been read

        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();
        
        Serial.print("\n\n\n");
        
        actionState = AS_IDLE;
    
      }
    }
    break;
  }
}





