#pragma once
#include <Arduino.h>

// Khởi tạo hệ thống cấu hình (Preferences, Access Point WiFi, WebServer & DNS Captive Portal)
void initConfigPortal();

// Xử lý các request từ web và DNS trong vòng lặp loop()
void handleConfigPortal();

// Lấy các giá trị cấu hình đã lưu trong Flash NVS
String config_get_ssid();
String config_get_password();
String config_get_telegram_token();
String config_get_telegram_chat_id();
int    config_get_goc_mo();
int    config_get_thoi_gian_giu();
int    config_get_muc_rac_day();

// Cập nhật trạng thái thời gian thực để hiển thị lên trang web
void config_set_realtime_status(float distance, bool isLidOpen);

