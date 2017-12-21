//硬件端程序

#include "./ESP8266.h"
#include <SoftwareSerial.h> 
#include "I2Cdev.h"   
#include <Wire.h>                   //无线
#include "SHT2x.h"                  //温度
#include <Adafruit_NeoPixel.h>      //小灯泡
//一堆可爱的头文件


#define PIN 6                       //LED的引脚
#define PIN_NUM 2                   //定义允许接的led灯的个数
#define  sensorPin_1  A0            //定义了光照的传感器的接口
const int buttonPin = D2;           //定义了按钮接口初始状态，不过这个功能没有实现，只是一个预留接口
#define humanHotSensor 4            //红外传感器接口
float buttonState = 0;              //按钮接口初始状态，不过这个功能没有实现，只是一个预留接口
bool humanHotState = false;         //红外传感器接口初始状态
#define INTERVAL_SENSOR   1000             //定义传感器采样时间间隔  597000
#define INTERVAL_NET      1000             //定义发送时间
#define INTERVAL_sensor 2000         
#define INTERVAL_OLED 1000
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIN_NUM, PIN, NEO_GRB + NEO_KHZ800); //该函数第一个参数控制串联灯的个数，第二个是控制用哪个pin脚输出，第三个显示颜色和变化闪烁频率
#define SSID           "AI"                //热点的账户   // cannot be longer than 32 characters!
#define PASSWORD       "88888888"          //热点的密码
#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data  received before closing the connection.  If you know the server you're accessing is quick to respond, you can reduce this value.
#define HOST_NAME   "api.heclouds.com"     
#define DEVICEID   "20470730"
#define PROJECTID "107031"
#define HOST_PORT   (80)
String apiKey="JTmypYgjhw2B=GcuqZhlmxjlpZc=";     //api
char buf[10];
unsigned long sensorlastTime = millis();
float tempOLED, humiOLED, lightnessOLED;
String mCottenData;
String jsonToSend;
float sensor_tem, sensor_hum, sensor_lux，sensor_butt;                    //传感器温度、湿度、光照   
char  sensor_tem_c[7], sensor_hum_c[7], sensor_lux_c[7] ,buttonState_c[7];    //换成char数组传输

SoftwareSerial mySerial(2, 3); /* RX:D3, TX:D2 */
ESP8266 wifi(mySerial);
//ESP8266 wifi(Serial1);                                      //定义一个ESP8266（wifi）的对象
unsigned long net_time1 = millis();                          //数据上传服务器时间
unsigned long sensor_time = millis();                        //传感器采样时间计时器
//int SensorData;                                   //用于存储传感器数据
String postString;                                //用于存储发送数据的字符串
//String jsonToSend;                                //用于存储发送的json格式参数

void setup(void)     //初始化函数  
{       
    strip.begin();              //对小灯泡初始化
    Wire.begin();               //对无线传输初始化
    Serial.begin(9600);        //波特率设置
    while(!Serial);
    pinMode(sensorPin_1, INPUT);
    pinMode(buttonPin, INPUT);
    pinMode(humanHotSensor, INPUT);   //上面三个是三个引脚设置为输入模式
    Serial.print("setup begin\r\n");   

  Serial.print("FW Version:");
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStationSoftAP()) {
    Serial.print("to station + softap ok\r\n");
  } else {
    Serial.print("to station + softap err\r\n");
  }

  if (wifi.joinAP(SSID, PASSWORD)) {      //加入无线网
    Serial.print("Join AP success\r\n");  
    Serial.print("IP: ");
    Serial.println(wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  if (wifi.disableMUX()) {
    Serial.print("single ok\r\n");
  } else {
    Serial.print("single err\r\n");
  }

  Serial.print("setup end\r\n");                    //上面三个是对目前硬件端接入情况在传口里面的打印情况
    
  
}
void loop(void)     //循环函数  ，由采样和发送两部分组成
{   
  if (sensor_time > millis())  
  sensor_time = millis();       //设置时间
    
  if(millis() - sensor_time > INTERVAL_SENSOR)              //传感器采样时间间隔  
  {  
    getSensorData();                                        //读串口中的传感器数据
    sensor_time = millis();
  }  

    
  if (net_time1 > millis())  
  net_time1 = millis();         //设置时间
  
  if (millis() - net_time1 > INTERVAL_NET)                  //发送数据时间间隔
  {                
    updateSensorData();                                     //将数据上传到服务器的函数
    net_time1 = millis();
  }
  
}

void getSensorData(){                 //采样的函数
    strip.setPixelColor(0, strip.Color(255, 0, 0));
    strip.show();                     //小灯泡亮起来的函数
    sensor_tem = SHT2x.GetTemperature() ;   //采集温度
    sensor_hum = SHT2x.GetHumidity();       //采集湿度
    sensor_lux = analogRead(A0);            //采集光强
    buttonState =digitalRead(buttonPin);    //采集按钮值，不过目前这个功能没有实现，在意图实现的过程中遇到了不可知的错误。有大神解决了的话，劳烦告诉我一声。
    humanHotState = digitalRead(humanHotSensor);//采集红外感应
    dtostrf(sensor_tem, 2, 1, sensor_tem_c);    //温度的传递
    dtostrf(sensor_hum, 2, 1, sensor_hum_c);    //湿度的传递
    dtostrf(sensor_lux, 3, 1, sensor_lux_c);    //光强的传递
    dtostrf(buttonState, 3, 1, buttonState_c);  //红外感应值的传递
}
void updateSensorData() {
  if (wifi.createTCP(HOST_NAME, HOST_PORT)) {      //建立TCP连接，如果失败，不能发送该数据
    strip.setPixelColor(0, strip.Color(0, 0, 0));//红光，这个红颜色似乎有点恐怖，毕竟小灯泡是亮在脑袋里面的，大家可以看着调......
    strip.show();   //LED显示
    Serial.print("create tcp ok\r\n");

    jsonToSend="{ \"Temperature\":";
    dtostrf(sensor_tem,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";      //打印温度
    
    jsonToSend+=",\"Humidity\":";
    dtostrf(sensor_hum,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";     //打印温度
    
    jsonToSend+=",\"Light\":";
     dtostrf(sensor_lux,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";    //打印温度
    
    jsonToSend+=",\"button\":";
     dtostrf(humanHotState,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";    //打印温度
    
    jsonToSend+="}";
    postString="POST /devices/";
    postString+=DEVICEID;
    postString+="/datapoints?type=3 HTTP/1.1";
    postString+="\r\n";
    postString+="api-key:";
    postString+=apiKey;
    postString+="\r\n";
    postString+="Host:api.heclouds.com\r\n";
    postString+="Connection:close\r\n";
    postString+="Content-Length:";
    postString+=jsonToSend.length();
    postString+="\r\n";
    postString+="\r\n";
    postString+=jsonToSend;
    postString+="\r\n";
    postString+="\r\n";
    postString+="\r\n";                               //上面这堆东西，大家看看就好。。。。。。大家开心就好，不要想他们了

  const char *postArray = postString.c_str();                 //将str转化为char数组
  Serial.println(postArray);
  wifi.send((const uint8_t*)postArray, strlen(postArray));    //send发送命令，参数必须是这两种格式，尤其是(const uint8_t*)
  Serial.println("send success");   
     if (wifi.releaseTCP()) {                                 //释放TCP连接
        Serial.print("release tcp ok\r\n");
        } 
     else {
        Serial.print("release tcp err\r\n");
        }
      postArray = NULL;                                       //清空数组，等待下次传输数据
  
  } else {
    Serial.print("create tcp err\r\n");
  }
  
}

