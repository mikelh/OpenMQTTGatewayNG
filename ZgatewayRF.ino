/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal

    Copyright: (c)Florian ROBERT
  
    This file is part of OpenMQTTGateway.
    
    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef ZgatewayRF
 
#include <RCSwitch64.h> // library for controling Radio frequency switch

RCSwitch64 mySwitch = RCSwitch64();

void setupRF(){

  //RF init parameters
  mySwitch.enableTransmit(RF_EMITTER_PIN);
  mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT); 
  mySwitch.enableReceive(RF_RECEIVER_PIN); 
  
}

boolean RFtoMQTT(){

  if (mySwitch.available()){
    trc(F("Rcv. RF"));
    unsigned long MQTTvalue = 0;
    unsigned long MQTTvalueH = 0;
    String MQTTprotocol;
    String MQTTbits;
    String MQTTlength;
    String value;
    String valueH;
    String hextopic;
    boolean result;
    unsigned bits;
    
    MQTTvalue = mySwitch.getReceivedValue();
    MQTTvalueH = mySwitch.getReceivedValueH();
    MQTTprotocol = String(mySwitch.getReceivedProtocol());
    bits = mySwitch.getReceivedBitlength();
    MQTTbits = String(bits);
    MQTTlength = String(mySwitch.getReceivedDelay());
    mySwitch.resetAvailable();
    if (bits <= 32) // old version applicable
    {
    if (!isAduplicate(MQTTvalue) && MQTTvalue!=0) {// conditions to avoid duplications of RF -->MQTT
        trc(F("Adv data RFtoMQTT"));
        client.publish(subjectRFtoMQTTprotocol,(char *)MQTTprotocol.c_str());
        client.publish(subjectRFtoMQTTbits,(char *)MQTTbits.c_str());    
        client.publish(subjectRFtoMQTTlength,(char *)MQTTlength.c_str());    
        trc(F("Sending RFtoMQTT"));
        value = String(MQTTvalue);
        trc(value);
        client.publish(subjectRFtoMQTT,(char *)value.c_str());
 
        hextopic = String(subjectRFtoMQTT) + "/" + String(MQTTvalue&0xffffff, HEX);
        value = String(MQTTvalue >> 24, HEX);       
        client.publish((char *)hextopic.c_str(),(char *)(value).c_str());

        hextopic = String(subjectRFtoMQTT) + "/" + MQTTprotocol;
        value = String(MQTTvalue);
        client.publish((char *)hextopic.c_str(),(char *)value.c_str());
        
        // value = "0x" + String(MQTTvalue, HEX);
        value = String(MQTTvalue, HEX);
        hextopic = String(subjectRFtoMQTT) + "/HEX";
        value = MQTTprotocol + String(MQTTvalue,HEX);       
        result = client.publish((char *)hextopic.c_str(),(char *)value.c_str());
 
        if (repeatRFwMQTT){
            trc(F("Publish RF for repeat"));
            client.publish(subjectMQTTtoRF,(char *)value.c_str());
        }
        return result;
    } 
  
  return false;
 } else { // more than 32 bits in protocol
    if ((!isAduplicate(MQTTvalue) || !isAduplicate(MQTTvalueH)) && (MQTTvalue | MQTTvalueH) !=0) {// conditions to avoid duplications of RF -->MQTT
        trc(F("Adv data RFtoMQTTH"));
        client.publish(subjectRFtoMQTTprotocol,(char *)MQTTprotocol.c_str());
        client.publish(subjectRFtoMQTTbits,(char *)MQTTbits.c_str());    
        client.publish(subjectRFtoMQTTlength,(char *)MQTTlength.c_str());    
        trc(F("Sending RFtoMQTTH"));
        value = String(MQTTvalue);
        valueH = String(MQTTvalueH);
        trc(value); trc(valueH); trc(MQTTlength);

// encode upper 32 bits into protocol topic along with protocol id leaving room for 32 bits of data in the data value
// eg. .../85/3783316790 for a WS1700 sensor using protocol 8, header 0101 data 1.1.10.0001.10000000.11010101.00110110 (0xe180d536)
        hextopic = String(subjectRFtoMQTT) + "/" + MQTTprotocol + String(MQTTvalueH, HEX);
        client.publish((char *)hextopic.c_str(),(char *)value.c_str());
        
        hextopic = String(subjectRFtoMQTT) + "/HEX";
        value = MQTTprotocol + String(MQTTvalueH,HEX) + String(MQTTvalue,HEX);       
        result = client.publish((char *)hextopic.c_str(),(char *)value.c_str());

        if (repeatRFwMQTT){
            trc(F("Publish RF for repeat"));
            client.publish(subjectMQTTtoRF,(char *)value.c_str());
            client.publish(subjectMQTTtoRFH,(char *)valueH.c_str());
        }
        return result;
    } 
  }
  
 }
}

/*
int handleProtocol8()
{
int hum;
int temp;
int sw;
int batt;
int channel;
int device;
int header;

//Device: 41, Temp: 2345, Hum:41, Ch: 1, Sw: 1, Batt: 1
// 0101_11100001_1_0_00_000010111001_00101001

//78        id = binToDecRev(binary, 4, 11);
//79        battery = binary[12];
//80        temperature = ((double)binToDecRev(binary, 16, 27));
//81        if (binary[16] == 1) {
//82            temperature -= 4096;
//83        }
//84        humidity = (double)binToDecRev(binary, 28, 35)


  if( RFbitlength != 36) {
    return -1;  
    }
  hum = RFvalue & 0xff;
  RFvalue >>= 8;
  temp = RFvalue & 0xfff;
  if (temp & 0x400) {
    temp = temp - 4096;
    }
  RFvalue >>= 12;
  channel = RFvalue & 0x3;
  RFvalue >>= 2;
  sw= RFvalue & 0x1;
  RFvalue >>= 1;
  batt = RFvalue & 0x1;
  RFvalue >>= 1;
  device= RFvalue &0xff;
  RFvalue >>= 8;
  header = RFvalue & 0xf;
  
  if (header != 5) {
    return -1;
  }
// construct JSON string and transmit
  Serial.print(F("WS1700: "));
  Serial.print(F("Device: "));
  Serial.print(device);
  Serial.print(F(", Temp: "));
  Serial.print(temp);
  Serial.print(F(", Hum:"));
  Serial.print(hum);
  Serial.print(F(", Ch: "));
  Serial.print(channel);
  Serial.print(F(", Sw: "));
  Serial.print(sw);
  Serial.print(F(", Batt: "));
  Serial.println(batt);
  
  return 0;
 
}

*/


void MQTTtoRF(char * topicOri, char * datacallback) {

#if 1
  unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
#else
  unsigned long data = strtoul(datacallback, NULL, 0); // we will accept any valid numerical format <4294967296 (dec)
#endif  
  // RF DATA ANALYSIS
  //We look into the subject to see if a special RF protocol is defined 
  String topic = topicOri;
  int valuePRT = 0;
  int valuePLSL  = 0;
  int valueBITS  = 0;
  int pos = topic.lastIndexOf(RFprotocolKey);       
  if (pos != -1){
    pos = pos + +strlen(RFprotocolKey);
    valuePRT = (topic.substring(pos,pos + 1)).toInt();
    trc(F("RF Protocol:"));
    trc(String(valuePRT));
  }
  //We look into the subject to see if a special RF pulselength is defined 
  int pos2 = topic.lastIndexOf(RFpulselengthKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RFpulselengthKey);
    valuePLSL = (topic.substring(pos2,pos2 + 3)).toInt();
    trc(F("RF Pulse Lgth:"));
    trc(String(valuePLSL));
  }
  int pos3 = topic.lastIndexOf(RFbitsKey);       
  if (pos3 != -1){
    pos3 = pos3 + strlen(RFbitsKey);
    valueBITS = (topic.substring(pos3,pos3 + 2)).toInt();
    trc(F("Bits nb:"));
    trc(String(valueBITS));
  }
  
  if ((topic == subjectMQTTtoRF) && (valuePRT == 0) && (valuePLSL  == 0) && (valueBITS == 0)){
    trc(F("MQTTtoRF dflt"));
    mySwitch.setProtocol(1,350);
    mySwitch.send(data, 24);
    // Acknowledgement to the GTWRF topic
    boolean result = client.publish(subjectGTWRFtoMQTT, datacallback);
    if (result)trc(F("Ack pub."));
    
  } else if ((valuePRT != 0) || (valuePLSL  != 0)|| (valueBITS  != 0)){
    trc(F("MQTTtoRF usr par."));
    if (valuePRT == 0) valuePRT = 1;
    if (valuePLSL == 0) valuePLSL = 350;
    if (valueBITS == 0) valueBITS = 24;
    trc(String(valuePRT));
    trc(String(valuePLSL));
    trc(String(valueBITS));
    mySwitch.setProtocol(valuePRT,valuePLSL);
    mySwitch.send(data, valueBITS);
    // Acknowledgement to the GTWRF topic 
    boolean result = client.publish(subjectGTWRFtoMQTT, datacallback);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
    if (result){
      trc(F("MQTTtoRF ack pub."));
      trc(String(data));
      };
  }
  
}
#endif
