/*
 * ESP32 C3 + ML307A短信转发到企业微信机器人
 * 作者：ThuHub
 * 日期：2025.11.22
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

//            看门狗配置
#define WATCHDOG_TIMEOUT_MS 30000
hw_timer_t *watchdogTimer = NULL;
volatile bool watchdogFed = true;
void IRAM_ATTR watchdogInterrupt() {
    if (!watchdogFed) {
        Serial.println("看门狗超时，系统即将重启...");
        ESP.restart();
    }
    watchdogFed = false;
}

void feedWatchdog() {
    watchdogFed = true;
    timerWrite(watchdogTimer, 0);
}

void initWatchdog() {
    Serial.println("初始化看门狗定时器...");
    watchdogTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(watchdogTimer, &watchdogInterrupt, true);
    timerAlarmWrite(watchdogTimer, WATCHDOG_TIMEOUT_MS * 1000, true);
    timerAlarmEnable(watchdogTimer);
    Serial.println("看门狗初始化完成，超时时间：" + String(WATCHDOG_TIMEOUT_MS) + "ms");
}

//           用户配置
const char* wifi_ssid = "你的WiFi名称";
const char* wifi_password = "你的WiFi密码";
const char* wechat_webhook = "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=你的机器人key";

//           硬件引脚定义
#define MODEM_TX_PIN 6    // ESP32的GPIO6连接模块TX
#define MODEM_RX_PIN 7    // ESP32的GPIO7连接模块RX
#define MODEM_PWR_PIN 8   // 模块电源控制（可选）

//           全局变量定义
String current_sender = "";
String current_content = "";
String current_time = "";
bool modem_ready = false;
bool wifi_connected = false;
unsigned long last_check_time = 0;

//           串口初始化
HardwareSerial ModemSerial(1);

//           函数声明
void setup_wifi();
void setup_modem();
bool send_at_command(String command, String expected_response, int timeout_ms);
void check_new_sms();
void parse_sms_message(String raw_data);
void forward_to_wechat(String sender, String content, String time);
void delete_sms(int index);
void reset_system();

//           主程序开始
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("========================================");
  Serial.println("   ESP32 C3 短信转发系统启动");
  Serial.println("========================================");
  initWatchdog();
  Serial.println("[1/4] 初始化ML307A模块串口...");
  ModemSerial.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  Serial.println("[2/4] 连接WiFi网络...");
  setup_wifi();
  Serial.println("[3/4] 初始化ML307A模块...");
  setup_modem();
  feedWatchdog();
  Serial.println("[4/4] 系统初始化完成！");
  Serial.println("等待接收短信...");
  Serial.println("========================================");
}

void loop() {
  feedWatchdog();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi连接断开，尝试重连...");
    setup_wifi();
  }
  if (millis() - last_check_time > 30000) {
    if (!send_at_command("AT", "OK", 1000)) {
      Serial.println("ML307A模块无响应，尝试重新初始化...");
      setup_modem();
    }
    last_check_time = millis();
  }
  if (ModemSerial.available()) {
    String response = ModemSerial.readString();
    Serial.print("收到模块数据: ");
    Serial.println(response);

    if (response.indexOf("+CMTI:") != -1) {
      Serial.println("检测到新短信通知！");
      check_new_sms();
    }
  }
  delay(100);
}

void setup_wifi() {
  Serial.print("正在连接WiFi: ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts % 4 == 0) {
      feedWatchdog();
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    Serial.println("");
    Serial.println("WiFi连接成功!");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
  } else {
    wifi_connected = false;
    Serial.println("");
    Serial.println("WiFi连接失败!");
  }
}

//           初始化ML307A模块
void setup_modem() {
  Serial.println("开始初始化ML307A模块...");
  while (ModemSerial.available()) {
    ModemSerial.read();
  }
  delay(1000);

  if (!send_at_command("AT", "OK", 2000)) {
    Serial.println("错误：ML307A模块无响应，请检查接线！");
    modem_ready = false;
    return;
  }
  Serial.println("1. 模块响应正常");
  send_at_command("ATE0", "OK", 1000);
  if (!send_at_command("AT+CMGF=1", "OK", 2000)) {
    Serial.println("错误：设置短信模式失败");
    modem_ready = false;
    return;
  }
  Serial.println("2. 短信模式设置为文本");
  send_at_command("AT+CNMI=2,1,0,0,0", "OK", 1000);
  if (!send_at_command("AT+CPIN?", "READY", 3000)) {
    Serial.println("警告：SIM卡未就绪，请检查SIM卡");
  }
  Serial.println("3. SIM卡检查完成");
  delay(3000);
  ModemSerial.println("AT+CREG?");
  delay(1000);
  String response = "";
  while (ModemSerial.available()) {
    response += ModemSerial.readString();
  }
  
  if (response.indexOf("0,1") != -1 || response.indexOf("0,5") != -1) {
    Serial.println("4. 网络注册成功");
    modem_ready = true;
  } else {
    Serial.println("4. 网络注册中...");
    Serial.println("响应: " + response);
    modem_ready = true;
  }
  Serial.println("ML307A模块初始化完成");
}

//           发送AT指令函数
bool send_at_command(String command, String expected_response, int timeout_ms) {
  Serial.print("发送AT指令: ");
  Serial.println(command);
  ModemSerial.println(command);
  unsigned long start_time = millis();
  String response = "";
  while (millis() - start_time < timeout_ms) {
    if (millis() - start_time > 500) {
      feedWatchdog();
    }
    if (ModemSerial.available()) {
      char c = ModemSerial.read();
      response += c;
      if (response.indexOf(expected_response) != -1) {
        return true;
      }
    }
    delay(1);
  }
  Serial.print("超时，收到: ");
  Serial.println(response);
  return false;
}

//           检查新短信
void check_new_sms() {
  Serial.println("正在读取新短信...");
  ModemSerial.println("AT+CMGL=\"REC UNREAD\"");
  delay(2000);
  feedWatchdog();
  String sms_data = "";
  while (ModemSerial.available()) {
    sms_data += ModemSerial.readString();
  }
  if (sms_data.length() > 0) {
    Serial.println("收到短信数据");
    parse_sms_message(sms_data);
  } else {
    Serial.println("未收到短信数据");
  }
}

//           解析短信内容
void parse_sms_message(String raw_data) {
  Serial.println("开始解析短信...");
  int sms_start = raw_data.indexOf("+CMGL:");
  if (sms_start == -1) {
    Serial.println("未找到短信数据");
    return;
  }
  int index_end = raw_data.indexOf(",", sms_start);
  String sms_index = raw_data.substring(sms_start + 6, index_end);
  sms_index.trim();
  Serial.print("短信索引: ");
  Serial.println(sms_index);
  int sender_start = raw_data.indexOf("\"+", index_end);
  int sender_end = raw_data.indexOf("\"", sender_start + 2);
  current_sender = raw_data.substring(sender_start + 1, sender_end);
  Serial.print("发送者: ");
  Serial.println(current_sender);
  int time_start = raw_data.indexOf("\"", sender_end + 1) + 1;
  int time_end = raw_data.indexOf("\"", time_start);
  current_time = raw_data.substring(time_start, time_end);
  Serial.print("时间: ");
  Serial.println(current_time);
  int content_start = raw_data.indexOf("\n", time_end) + 1;
  int content_end = raw_data.indexOf("OK", content_start);
  if (content_end == -1) {
    content_end = raw_data.length();
  }
  current_content = raw_data.substring(content_start, content_end);
  current_content.trim();
  Serial.print("内容: ");
  Serial.println(current_content);
  forward_to_wechat(current_sender, current_content, current_time);
  delete_sms(sms_index.toInt());
}

//           转发到企业微信
void forward_to_wechat(String sender, String content, String time) {
  if (!wifi_connected) {
    Serial.println("无法转发：WiFi未连接");
    return;
  }
  Serial.println("准备转发到企业微信...");
  feedWatchdog();
  HTTPClient http;
  http.begin(wechat_webhook);
  http.addHeader("Content-Type", "application/json");
  String wechat_message = "📱 收到新短信\\n";
  wechat_message += "👤 发件人: " + sender + "\\n";
  wechat_message += "🕐 时间: " + time + "\\n";
  wechat_message += "📄 内容: " + content;
  String json_data = "{\"msgtype\":\"text\",\"text\":{\"content\":\"" + wechat_message + "\"}}";
  Serial.print("发送JSON数据: ");
  Serial.println(json_data);
  int http_code = http.POST(json_data);
  feedWatchdog();

  if (http_code > 0) {
    if (http_code == HTTP_CODE_OK) {
      String response = http.getString();
      Serial.print("转发成功！响应: ");
      Serial.println(response);
    } else {
      Serial.print("转发成功，HTTP代码: ");
      Serial.println(http_code);
    }
  } else {
    Serial.print("转发失败，错误: ");
    Serial.println(http.errorToString(http_code));
  }
  http.end();
}

//           删除短信
void delete_sms(int index) {
  Serial.print("删除短信，索引: ");
  Serial.println(index);
  String command = "AT+CMGD=" + String(index);
  send_at_command(command, "OK", 2000);
}

//           系统重置
void reset_system() {
  Serial.println("系统出现错误，正在重置...");
  delay(1000);
  ESP.restart();
}