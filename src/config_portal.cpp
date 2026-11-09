#include "config_portal.h"
#include "web_page.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

static WebServer server(80);
static DNSServer dnsServer;
static Preferences prefs;

static String cfg_ssid = "";
static String cfg_pass = "";
static String cfg_token = "";
static String cfg_chatid = "";
static int cfg_goc_mo = 100;
static int cfg_thoi_gian_giu = 4000;
static int cfg_muc_rac_day = 10;

static float rt_distance = 0.0;
static bool rt_isLidOpen = false;

static bool shouldRestart = false;
static unsigned long restartTime = 0;

static void loadPreferences() {
  prefs.begin("thungrac", true); // Chế độ chỉ đọc (Read-only)
  cfg_ssid          = prefs.getString("ssid", "");
  cfg_pass          = prefs.getString("pass", "");
  cfg_token         = prefs.getString("token", "");
  cfg_chatid        = prefs.getString("chatid", "");
  cfg_goc_mo        = prefs.getInt("goc_mo", 100);
  cfg_thoi_gian_giu = prefs.getInt("tg_giu", 4000);
  cfg_muc_rac_day   = prefs.getInt("muc_day", 10);
  prefs.end();
}

static void handleRoot() {
  String html = PAGE_CONFIG_HTML;
  html.replace("%WIFI_SSID%", cfg_ssid);
  html.replace("%WIFI_PASS%", cfg_pass);
  html.replace("%TG_TOKEN%", cfg_token);
  html.replace("%TG_CHATID%", cfg_chatid);
  html.replace("%GOC_MO%", String(cfg_goc_mo));
  html.replace("%TG_GIU%", String(cfg_thoi_gian_giu / 1000));
  html.replace("%MUC_DAY%", String(cfg_muc_rac_day));

  server.send(200, "text/html", html);
}

static void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

static void handleStatus() {
  String ipStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  String json = "{";
  json += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"distance\":" + String(rt_distance, 1) + ",";
  json += "\"lid_open\":" + String(rt_isLidOpen ? "true" : "false") + ",";
  json += "\"ip\":\"" + ipStr + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

static void handleSave() {
  if (server.hasArg("ssid"))   cfg_ssid = server.arg("ssid");
  if (server.hasArg("pass"))   cfg_pass = server.arg("pass");
  if (server.hasArg("token"))  cfg_token = server.arg("token");
  if (server.hasArg("chatid")) cfg_chatid = server.arg("chatid");

  if (server.hasArg("goc_mo")) {
    int g = server.arg("goc_mo").toInt();
    if (g >= 60 && g <= 150) cfg_goc_mo = g;
  }
  if (server.hasArg("tg_giu")) {
    int t = server.arg("tg_giu").toInt();
    if (t >= 2 && t <= 20) cfg_thoi_gian_giu = t * 1000;
  }
  if (server.hasArg("muc_day")) {
    int m = server.arg("muc_day").toInt();
    if (m >= 4 && m <= 50) cfg_muc_rac_day = m;
  }

  // Lưu vào Flash NVS
  prefs.begin("thungrac", false); // Chế độ ghi (Read-Write)
  prefs.putString("ssid", cfg_ssid);
  prefs.putString("pass", cfg_pass);
  prefs.putString("token", cfg_token);
  prefs.putString("chatid", cfg_chatid);
  prefs.putInt("goc_mo", cfg_goc_mo);
  prefs.putInt("tg_giu", cfg_thoi_gian_giu);
  prefs.putInt("muc_day", cfg_muc_rac_day);
  prefs.end();

  server.send(200, "text/plain", "OK");

  Serial.println("\n>> [CONFIG] Da luu cau hinh moi thanh cong! Chuan bi khoi dong lai...");
  shouldRestart = true;
  restartTime = millis();
}

static void handleNotFound() {
  // Điều hướng Captive Portal về trang cấu hình
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

void initConfigPortal() {
  loadPreferences();

  // Khởi động chế độ phát WiFi (Access Point)
  WiFi.mode(WIFI_AP_STA);
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("ThungRac-ThongMinh"); // Tên WiFi của thùng rác

  Serial.println(">> [AP WIFI] Da phat WiFi: ThungRac-ThongMinh");
  Serial.print(">> [AP WIFI] Dia chi IP trang cau hinh: http://");
  Serial.println(WiFi.softAPIP());

  // Bắt đầu DNS Server chuyển hướng tự động (Captive Portal)
  dnsServer.start(53, "*", apIP);

  // Đăng ký các route WebServer
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/save", HTTP_POST, handleSave);

  // Các route kiểm tra mạng của Android / Apple / Windows để tự mở Captive Portal
  server.on("/generate_204", HTTP_GET, handleRoot);
  server.on("/hotspot-detect.html", HTTP_GET, handleRoot);
  server.on("/canonical.html", HTTP_GET, handleRoot);
  server.on("/ncsi.txt", HTTP_GET, handleRoot);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println(">> [WEBSERVER] May chu web cau hinh da san sang hoat dong!");
}

void handleConfigPortal() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (shouldRestart && (millis() - restartTime > 1500)) {
    Serial.println(">> [RESTART] Dang khoi dong lai ESP32...");
    ESP.restart();
  }
}

String config_get_ssid()             { return cfg_ssid; }
String config_get_password()         { return cfg_pass; }
String config_get_telegram_token()   { return cfg_token; }
String config_get_telegram_chat_id() { return cfg_chatid; }
int    config_get_goc_mo()           { return cfg_goc_mo; }
int    config_get_thoi_gian_giu()    { return cfg_thoi_gian_giu; }
int    config_get_muc_rac_day()      { return cfg_muc_rac_day; }

void config_set_realtime_status(float distance, bool isLidOpen) {
  rt_distance = distance;
  rt_isLidOpen = isLidOpen;
}

