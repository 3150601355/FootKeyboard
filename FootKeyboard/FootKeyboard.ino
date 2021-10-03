/*
   作者：B站@偶尔有点小迷糊 
        https://space.bilibili.com/39665558


   本代码参考自：
   esp32官方例子及第三方网站：https://randomnerdtutorials.com/esp32-web-server-arduino-ide/


   2021.10


   小白警告：需要先搭建好适合于arduino的esp32的开发环境，否则编译出错。
*/

#include <WiFi.h>
#include <BleKeyboard.h>

// 无线网的用户名和密码
const char* ssid = "此处改为你家的无线热点名称";
const char* password = "热点的密码";

// 创建server对象，80是端口号
WiFiServer server(80);

// 存放HTTP request
String header = "";

// 用于判断网络是否超时，设为2000ms
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// 当前使用的快捷键的序号
int keyIndex = 0;
// 快捷键文字描述列表
String arrKeyTitle[] = {"Ctr + F5", "Win + Tab", "Ctrl + V", "Shift键", "生成解决方案", "VS注释代码", "切换桌面"};

unsigned int arrKeyCode[][3] = { // -1表示此位置无按键，为了演示追求代码简洁（用list来表示快捷键逻辑更清晰且易扩展）
                                {KEY_LEFT_CTRL,   KEY_F5,       -1},
                                {KEY_LEFT_GUI,    KEY_TAB,      -1},
                                {KEY_LEFT_CTRL,   'v',          -1},
                                {KEY_LEFT_SHIFT,  -1,           -1},
                                {KEY_LEFT_CTRL,   KEY_LEFT_SHIFT, 'b'},
                                {KEY_LEFT_CTRL,   'k',          'c'},
                                {KEY_LEFT_CTRL,   KEY_LEFT_GUI, KEY_RIGHT_ARROW},
                              };
                              
//上面数组含有的快捷键个数
const int count = sizeof(arrKeyCode) / sizeof(arrKeyCode[0]);

// 蓝牙键盘相关
BleKeyboard bleKeyboard;
#define BUTTON_1 27       // 我把按钮接在开发板的27号引脚，可根据自己情况修改
unsigned int preState = HIGH;

void setup() {
  // 初始化串口
  Serial.begin(115200);

  // 初始化IO，输入模式，上拉（即默认为高电平）
  pinMode(BUTTON_1, INPUT_PULLUP);

  // 初始化蓝牙键盘
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();


  // 连接WiFi
  Serial.print("正在连接：");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi已连接. IP地址: ");
  Serial.println(WiFi.localIP());

  // 启动网络服务
  server.begin();
}

void sendResponse(WiFiClient &client) {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  // 根据客户端请求的地址，设置keyIndex
  Serial.println(header);
  for (int i = 0; i < count; i++) {
    String strIndex = String(i);
    if (header.indexOf("GET /" + strIndex) >= 0) {  // e.g "GET /5"
      keyIndex = i;
    }
  }

  // 生成网页内容
  // 头
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta charset=\"utf-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">");

  // CSS
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".button { background-color: #FF6600; border: none; color: white; padding: 16px 40px; width:50%;border-radius:12px;");
  client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
  client.println(".button2 {background-color: #555555;border-radius:12px;}</style></head>");

  // 标题
  client.println("<body><h1>脚控键盘按键配置</h1>");
  client.println("<body><h4>作者：B站@偶尔有点小迷糊</h4>");

  // 按钮
  for (int i = 0; i < count; i++) {
    if (i == keyIndex) {
      client.println("<p><a href=\"/" + String(i) + "\"><button class=\"button\">" + arrKeyTitle[i] + " 已启用</button></a></p>");
    }
    else {
      client.println("<p><a href=\"/" + String(i) + "\"><button class=\"button button2\">" + arrKeyTitle[i] + "</button></a></p>");
    }
  }

  // 内容结束，收工
  client.println("</body></html>");

  // 再来个空行表示响应结束
  client.println();
}

void loop() {
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    unsigned long currentTime = millis();
    previousTime = currentTime;
    Serial.println("有新的连接");

    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            sendResponse(client);
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    
    // Clear the header variable
    header = "";
    
    // Close the connection
    client.stop();
    Serial.println("连接断开\n");
  }


  if (bleKeyboard.isConnected()) {
    // 读取按钮电平状态，但不用电平触发，使用边沿触发，更加稳定可靠
    int buttonState = digitalRead(BUTTON_1);

    if (buttonState == LOW && preState == HIGH) {
      Serial.println("按（踩）下按键");

      for (int m = 0; m < 3; m++) {
        if (-1 != arrKeyCode[keyIndex][m])
          bleKeyboard.press(arrKeyCode[keyIndex][m]);
      }

      preState = LOW;
      delay(20);
    }
    else if (buttonState == HIGH && preState == LOW) {
      Serial.println("释放按键");

      for (int m = 0; m < 3; m++) {
        if (-1 != arrKeyCode[keyIndex][m])
          bleKeyboard.release(arrKeyCode[keyIndex][m]);
      }

      preState = HIGH;
    }
  }

  delay(100);
}
