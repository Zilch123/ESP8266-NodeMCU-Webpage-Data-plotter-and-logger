/*------------------------------------------------------------------------------
  11/09/2020
  Author: Timoth Dev 
  Platforms: ESP8266
  Language: C++/Arduino + HTML/JS
  File: ESP_plot.ino
  ------------------------------------------------------------------------------
  Description: 
  Input: ADS1115 chip gets differential analog input to SPI.
  Output: HTML JS webpage to plot and download streamed data.  
------------------------------------------------------------------------------*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Ticker.h>
#include <Wire.h>
int16_t results;
// Connecting to the Internet
char * ssid = "Your_WIFI_SSID";
char * password = "WIFI_Password";

// Running a web server
ESP8266WebServer server;

// Adding a websocket to the server
WebSocketsServer webSocket = WebSocketsServer(81);

// Serving a web page (from flash memory)
// formatted as a string literal
char webpage[] PROGMEM = R"=====(
<html>
<!-- Adding a data chart using Chart.js -->
<head>
  <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
</head>
<body onload="javascript:init()">
<tr>
    <td><input type="button" id="mixBut" value="Start" /></td>
    <td><input type="button" id="clearBtn" onclick="clear_()" value="Clear"/></td>
    <td>Filename:</td>
    <td><input id="inputFileNameToSaveAs"></input></td>
    <td><button onclick="saveTextAsFile()">Download</button></td>
</tr>
<hr />
<div id="graph"></div>
<!-- Adding a websocket to the client (webpage) -->
<script>
  const df=[]
  const df_t=[]
  var mixBut = document.getElementById("mixBut");
  mixBut.addEventListener("click", Start);
  var webSocket, dataPlot;
  var maxDataPoints = 200;
  var recordFlag = 0;
  var data_empty = [{
    x: [],
    y: [],
    mode: 'lines',
    line: {color: '#80CAF6'}
  }];
  Plotly.plot('graph', data_empty );  

  function init() {
    webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
    webSocket.onmessage = function(event) {
      var data = JSON.parse(event.data);
      var today = new Date();
      var t = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds() + ":" + today.getMilliseconds();
      var update = {
                    x:  [[t]],
                    y: [[data.value]]
                   }
  
      if(recordFlag==1)
      {
        df_t.push(t);
        df.push(data.value);
        Plotly.extendTraces('graph', update, [0]);
      }
      
    }
  }
  function saveTextAsFile(){
//    console.log("heyThere");
    var recording2Save = [];
    console.log(df.length, df_t.length);
    for (var i=0; i<df.length; i++){
     recording2Save[i] = [df_t[i],"\t", df[i], "\r\n"]; 
    }
    var textToSave = "Time\tData\r\n"+ recording2Save.toString().split(",").join("");
    var textToSaveAsBlob = new Blob([textToSave], {type:"text/plain"});
    var textToSaveAsURL = window.URL.createObjectURL(textToSaveAsBlob);
    var fileNameToSaveAs = document.getElementById("inputFileNameToSaveAs").value;
 
    var downloadLink = document.createElement("a"); 
    downloadLink.download = fileNameToSaveAs;
    downloadLink.innerHTML =  "Download File";
    downloadLink.href = textToSaveAsURL;
    downloadLink.onclick = destroyClickedElement;
    downloadLink.style.display = "none";
    document.body.appendChild(downloadLink);
    downloadLink.click();
    
    df.splice(0,df.length);
    df_t.splice(0,df_t.length);
}

function Start(){
    console.log("Started");
    mixBut.removeEventListener("click", Start);
    mixBut.addEventListener("click", Stop);
    mixBut.value = "Stop";
    recordFlag = 1;
}

function Stop(){
    console.log("Stopped");
    mixBut.removeEventListener("click", Stop);
    mixBut.addEventListener("click", Start);
    mixBut.value = "Start";
    recordFlag = 0;
}

function clear_()
{
  df.splice(0,df.length);
  df_t.splice(0,df_t.length);
}

function destroyClickedElement(event)
{
    document.body.removeChild(event.target);
}
</script>
</body>
</html>
)=====";

void setup() {
  WiFi.begin(ssid, password);
   Serial.begin(115200);
  while(WiFi.status()!=WL_CONNECTED) {
    // Serial.print(".");
    delay(500);
  }
  // Serial.println("");
   Serial.print("IP Address: ");
   Serial.println(WiFi.localIP());

  server.on("/",[](){
    server.send_P(200, "text/html", webpage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}
void loop() {
  webSocket.loop();
  server.handleClient();
//  Read sensor data and broacast it 
  results = analogRead();  
  String json = "{\"value\":";
  json += results;
  json += "}";
  webSocket.broadcastTXT(json.c_str(), json.length());
  delay(5);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  // Do something with the data from the client
  if(type == WStype_TEXT){
    float dataRate = (float) atof((const char *) &payload[0]);
  }
}
