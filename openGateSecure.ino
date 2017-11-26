
// --------------Open start-s3-XL controlled gate---------------------------
//---------------Securly via the internet connection------------------------
// --------------For NodeMcu 1.0 OR esp8266-12e micro-controllers-----------

#include <sha256.h>
#include <ESP8266WiFi.h>

#define GATE_PIN 2                     // connects to gate pin 2
#define PORT 80                        // tcp/ip http port
const String GATE_PASSWD = "password"; // To-Do: change to real password

char ssid[] = "mywifissid"; // home WiFi name
char pass[] = "mywifipassword";  // home Wifi WPA2 password

IPAddress IP(192, 168, 1, 255);     // To-Do: change to correct ip
IPAddress gateway(192, 168, 1, 1);  //To-Do: change to correct ip
IPAddress subnet(255, 255, 255, 0); //  To-Do: change to correct subnet
WiFiServer server(PORT);             

int saltCount = 173421; // Dynamic shared public salt
uint8_t *hash;


// runs once at the begining of the program
// Sets micro-controller and intilizes internet connection 
void setup() {
  Serial.begin(9600); // For logging perpous
  
  pinMode(GATE_PIN, OUTPUT);
  digitalWrite(GATE_PIN, HIGH);
  
  WiFi.config(IP,gateway,subnet); 
  WiFi.begin(ssid, pass);
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  
  while (WiFi.status() != WL_CONNECTED) {    // wait for wifi connection
    Serial.print(".");
    delay(500);
  }
  
   printWifiStatus();
   server.begin();
}  


// runs forever: waits for an http request from a client and responds accordingly
// if HTTP request is GET server responds with html page
// if HTTP request is POST compares clients hash to the server's and opens/pauses/closes gate when they are equal
void loop() {
    
  // get an available client
  WiFiClient client = server.available(); // ----------blocking----------
  
  //http request
  String request = "";
  // buffer for incoming characters
  char buffer;
  
  if(client){ 
    Serial. println("\nconnected");
    
    if(client.connected()){
      int i = 0;
      while(client.available()<=0 && i < 3000) { // wait max 3 seconds for client to send data.
        delay(1);
        i++;
        } 
        
      if (client.available() > 0){ // client has sent DATA
        Serial.println("client-sent-data");
        
      while(((buffer = client.read()) !='\n') || (request == "")) {  // read only first line of request or wait for data
        request += buffer;
      }
      Serial.print(request);
        
      if (request.indexOf("GET / HTTP/1.1") != -1 ) { // serve client spesific page 
        
        // throw away other client data
        client.flush();
        
        //increaments salt so hash will be different
        String page = serverResponse(String(++saltCount, HEX)); 
        client.print(page);
        delay(1);
        
        // end client connection
        client.stop();
      }
      
      if (request.indexOf("POST") != -1){ // handle this request as a post request
                         
        while (!request.startsWith("\r\n")){ // flush all http headers line by line (ends with empty line)
          request = "";                      
          while(!request.endsWith("\r\n")){
            buffer = client.read();
            request += buffer;
          }
          Serial.print(request);
        }  
        
        
        String clientHash = "";
        for (int i = 0 ; i <64 ; i ++) { // read only hash from the 11th line
          buffer = client.read();
          clientHash += buffer;
        }
        Serial.println(clientHash);
        
        //password with cuurent salt combined
        String currentPass = (GATE_PASSWD+String(saltCount, HEX));
        
        // --------------create hash from currentPassword---------------
        BYTE hash[SHA256_BLOCK_SIZE];
        char serverHash[2*SHA256_BLOCK_SIZE+1];
        Sha256* test = new Sha256();
        unsigned char * text = (unsigned char*) currentPass.c_str();
        test->update(text, strlen((const char*)text));
        test->final(hash);
        for(int i=0; i<SHA256_BLOCK_SIZE; ++i)
          sprintf(serverHash+2*i, "%02X", hash[i]);
        // -------------------------------------------------------------
        
        Serial.println("pass is - " + currentPass);
        Serial.print("hash is - ");
        Serial.println(serverHash);
        
        if (String(serverHash) == clientHash) { // compair server's hash with clients hash
          client.print("<html>you shall pass</html>");
          openGate();
        }
        // end client connection
        client.stop();
      }
    }
   }
  }
}


// interface with start s3 XL gate controller
// keep connection high
// ground for a second in order to open gate 
void openGate(){
  if (digitalRead(GATE_PIN)== LOW)
    digitalWrite(GATE_PIN, HIGH);
  {
  digitalWrite(GATE_PIN, LOW);
  delay(1050);
  digitalWrite(GATE_PIN, HIGH);
  Serial.println("\n---High---");}
  
}

// creates an server response page for a http client
String serverResponse(String salt){
  
  const String HTTP_HEADER = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
  String html = "<html><head><Title>You Shall Not Pass</Title><link rel='icon' href='https://vignette.wikia.nocookie.net/fantendo/images/6/6e/Small-mario.png/revision/latest?cb=20120718024112'>"
                "</head><body><h1>sha256 of code+salt:</h1><br><p>will apear here</p>"
                "<script src='https://cdnjs.cloudflare.com/ajax/libs/crypto-js/3.1.2/components/core-min.js'></script>"
                "<script src='https://cdnjs.cloudflare.com/ajax/libs/crypto-js/3.1.9-1/sha256.js'></script>"
                "<script>"
                "function setCookie(name, value){"
                "document.cookie = name+'='+value+'; expires=Fri, 31 Dec 9999 23:59:59 GMT;';"
                "}"
                "function getCookie(name){"
                "var nameEQ = name+'=';"
                "var cookie = document.cookie;"
                "var pos = cookie.indexOf(nameEQ);"
                "if(pos < 0) return '';"
                "var end = cookie.indexOf(';',pos);"
                " if (end < 0 ) end = cookie.length;"
                "var Value = cookie.slice(pos+nameEQ.length, end);"
                "return Value;}"
                "var code = getCookie('code');"
                "var autoSignIn = getCookie('autoSignIn');"
                "if (code == '' || autoSignIn == '' || autoSignIn == 'false' || window.performance.navigation.type == 2 || window.performance.navigation.type == 255 ){"
                "code = prompt('please enter code' , code); "
                "if (code != null)"
                "setCookie('code',code);"
                "}"
                "var hash = CryptoJS.SHA256(String(code)+'";
  html += salt;
  html += "');"
          "document.getElementsByTagName('p')[0].innerHTML = hash;"
          "var xhr = new XMLHttpRequest(); xhr.open('POST', '/');" 
          "xhr.send(hash.toString().toUpperCase());"
          
          "</script>"
          "<style> input[type=checkbox] { zoom : 5} </style>"
          "<input type=\"checkbox\" id=\"check1\" onchange=\"setCookie('autoSignIn' , this.checked)\" checked = false > Auto Sign In </input>"
          "<script> var state = getCookie('autoSignIn'); if(state == '' || state == 'false') document.getElementById(\"check1\").checked = false;"
          "else if (state == 'true') document.getElementById(\"check1\").checked = true;"
          "</script>"
          "</body></html>";
  return HTTP_HEADER+html; // returns the page
}
  


    
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


