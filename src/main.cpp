#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config_portal.h"

// ================= CẤU HÌNH CHÂN KẾT NỐI =================
#define PIN_SERVO       13   // Cột D13 (Chân S) - Tín hiệu Servo MG995
#define PIN_IR_SENSOR   27   // Cột D27 (Chân S) - Cảm biến hồng ngoại
#define PIN_TRIG        26   // Cột D26 (Chân S) - Siêu âm TRIG
#define PIN_ECHO        25   // Cột D25 (Chân S) - Siêu âm ECHO

#define PIN_LED         32   // Cột D32: Đèn LED báo hiệu
#define PIN_BUZZER      33   // Cột D33: Còi báo hiệu (Buzzer)

// ================= THÔNG SỐ CÀI ĐẶT CƠ KHÍ =================
#define GOC_DONG             20   // Nâng điểm nghỉ đóng nắp lên 20 độ (để motor không bị cắm sâu quá)
#define TOC_DO_STEP          2    // Mỗi bước nhảy 2 độ
#define TOC_DO_DELAY         25   // Thời gian nghỉ giữa các bước (ms)
#define CANH_BAO_TRUOC_DONG  1000 // Cảnh báo trước khi đóng 1 giây (ms)

Servo myServo;
int currentAngle = GOC_DONG;

bool isLidOpen = false;
bool isPreCloseWarned = false;    // Đã phát cảnh báo sắp đóng nắp chưa
unsigned long lidOpenedTime = 0;
unsigned long lastSonarTime = 0;
bool isTrashFull = false;
float lastMeasuredDistance = 999.0;

// ================= HÀM PHÁT ÂM THANH GIAI ĐIỆU MARIO =================
void playNote(int freq, int durationMs) {
  if (freq > 0) {
    tone(PIN_BUZZER, freq, durationMs);
    digitalWrite(PIN_LED, HIGH);
    delay(durationMs);
    noTone(PIN_BUZZER);
    digitalWrite(PIN_LED, LOW);
  } else {
    delay(durationMs);
  }
  delay(25); // Khoảng nghỉ giữa các nốt
}

// 1. Âm thanh Mario ăn nấm (Power-Up) khi mở nắp
void soundMarioPowerUp() {
  int notes[]     = { 392, 494, 587, 784, 988, 1175 }; // G4, B4, D5, G5, B5, D6
  int durations[] = { 60,  60,  60,  60,  60,  90 };
  for (int i = 0; i < 6; i++) {
    playNote(notes[i], durations[i]);
  }
}

// 2. Âm thanh Mario thả cờ (Flagpole) nhẹ nhàng khi sắp đóng nắp
void soundMarioFlagpole() {
  int notes[]     = { 784, 659, 523, 392, 523 }; // G5, E5, C5, G4, C5
  int durations[] = { 70,  70,  70,  70,  110 };
  for (int i = 0; i < 5; i++) {
    playNote(notes[i], durations[i]);
  }
}

// 3. Âm thanh Mario Game Over khi thùng đầy hoặc từ chối mở nắp
void soundMarioGameOver() {
  int notes[]     = { 523, 392, 330, 440, 494, 440, 415, 466, 415, 392 };
  int durations[] = { 130, 130, 150, 110, 110, 110, 130, 130, 130, 320 };
  for (int i = 0; i < 10; i++) {
    playNote(notes[i], durations[i]);
  }
}

// Hàm phát tiếng bíp cơ bản dự phòng
void beep(int durationMs, int times = 1) {
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    digitalWrite(PIN_LED, HIGH);
    delay(durationMs);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED, LOW);
    if (times > 1) delay(80);
  }
}

// ================= HÀM GỬI THÔNG BÁO TELEGRAM =================
void sendTelegramMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(">> [TELEGRAM] Chua ket noi WiFi -> Bo qua gui tin nhan.");
    return;
  }

  String token = config_get_telegram_token();
  String chatId = config_get_telegram_chat_id();

  if (token.length() == 0 || chatId.length() == 0) {
    Serial.println(">> [TELEGRAM] Chua cau hinh Bot Token hoac Chat ID tren trang web.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Bỏ qua xác thực chứng chỉ SSL để gửi tin nhanh và nhẹ
  client.setTimeout(5000);

  HTTPClient https;
  String url = "https://api.telegram.org/bot" + token + "/sendMessage";

  Serial.println(">> [TELEGRAM] Dang gui thong bao den Telegram...");
  if (https.begin(client, url)) {
    https.addHeader("Content-Type", "application/json");
    String payload = "{\"chat_id\":\"" + chatId + "\",\"text\":\"" + message + "\"}";
    int httpCode = https.POST(payload);

    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
      Serial.println(">> [TELEGRAM] Gui thanh cong!");
    } else {
      Serial.printf(">> [TELEGRAM] Gui that bai! HTTP code: %d\n", httpCode);
    }
    https.end();
  } else {
    Serial.println(">> [TELEGRAM] Khong the ket noi api.telegram.org");
  }
}

// ================= HÀM ĐIỀU KHIỂN SERVO =================
// Hàm điều khiển Servo quay từ từ, êm ái đến góc mong muốn
// Có hỗ trợ kiểm tra vật cản (chống kẹp tay) khi đang đóng nắp
// Trả về: true nếu hoàn thành hết hành trình, false nếu bị ngắt do phát hiện vật cản
bool moveServoSlow(int targetAngle, bool checkObstacle = false) {
  if (currentAngle < targetAngle) {
    for (int pos = currentAngle; pos <= targetAngle; pos += TOC_DO_STEP) {
      myServo.write(pos);
      delay(TOC_DO_DELAY);
    }
  } else if (currentAngle > targetAngle) {
    for (int pos = currentAngle; pos >= targetAngle; pos -= TOC_DO_STEP) {
      // Nếu bật kiểm tra vật cản và phát hiện có tay/vật cản ở cảm biến IR
      if (checkObstacle && digitalRead(PIN_IR_SENSOR) == LOW) {
        currentAngle = pos; // Lưu vị trí góc hiện tại để đảo chiều mượt mà
        return false;       // Bị gián đoạn bởi vật cản
      }
      myServo.write(pos);
      delay(TOC_DO_DELAY);
    }
  }
  myServo.write(targetAngle);
  currentAngle = targetAngle;
  return true;
}

// ================= HÀM ĐO KHOẢNG CÁCH SIÊU ÂM =================
float getDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long duration = pulseIn(PIN_ECHO, HIGH, 25000);
  if (duration == 0) return 999.0;
  return (duration * 0.0343) / 2.0;
}

// ================= KHỞI TẠO HỆ THỐNG =================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== KHOI DONG HE THONG THUNG RAC THONG MINH ===");

  pinMode(PIN_IR_SENSOR, INPUT_PULLUP);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, LOW);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  myServo.setPeriodHertz(50);
  myServo.attach(PIN_SERVO, 500, 2400);

  // Đưa về vị trí đóng nắp nghỉ (20 độ)
  myServo.write(GOC_DONG);
  currentAngle = GOC_DONG;

  // Khởi động cổng cấu hình Web & WiFi AP Captive Portal
  initConfigPortal();

  // Kết nối đến mạng WiFi đã được cấu hình lưu trong Flash
  String savedSsid = config_get_ssid();
  String savedPass = config_get_password();

  if (savedSsid.length() > 0) {
    Serial.print(">> Dang ket noi WiFi nha ban: ");
    Serial.println(savedSsid);
    WiFi.begin(savedSsid.c_str(), savedPass.c_str());

    unsigned long startWifi = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startWifi < 5000)) {
      delay(200);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n>> WiFi ket noi thanh cong! IP nha ban: " + WiFi.localIP().toString());
    } else {
      Serial.println("\n>> Khong the ket noi WiFi nha ban. Vui long ket noi WiFi 'ThungRac-ThongMinh' de kiem tra lai!");
    }
  } else {
    Serial.println(">> Chua co WiFi nao duoc luu. Hay ket noi WiFi 'ThungRac-ThongMinh' de cai dat!");
  }

  // Phát nhạc Mario khởi động hệ thống
  soundMarioPowerUp();
  Serial.println(">> Khoi dong thanh cong! He thong da san sang.");
}

// ================= VÒNG LẶP CHÍNH =================
void loop() {
  // 0. XỬ LÝ YÊU CẦU WEB CẤU HÌNH & DNS CAPTIVE PORTAL
  handleConfigPortal();

  unsigned long now = millis();
  int irState = digitalRead(PIN_IR_SENSOR);

  // Lấy các thông số cấu hình động từ trang web
  int gocMo       = config_get_goc_mo();
  int thoiGianGiu = config_get_thoi_gian_giu();
  int mucRacDay   = config_get_muc_rac_day();

  // Cập nhật trạng thái thời gian thực lên web server
  config_set_realtime_status(lastMeasuredDistance, isLidOpen);

  // 1. NHẬN DIỆN TÍN HIỆU CẢM BIẾN HỒNG NGOẠI -> MỞ NẮP (CÓ BẢO VỆ CHỐNG TRÀN RÁC)
  if (irState == LOW && !isLidOpen) {
    // Nếu thùng rác đang báo đầy -> Kiểm tra lại và từ chối mở nếu vẫn đầy
    if (isTrashFull) {
      float currentDist = getDistance();
      lastMeasuredDistance = currentDist;
      if (currentDist <= mucRacDay && currentDist > 0.0) {
        Serial.println(">> [TU CHOI MO] Thung rac da DAY! Canh bao Game Over va khong mo nap.");
        soundMarioGameOver();        // Phát nhạc Game Over cảnh báo từ chối
        digitalWrite(PIN_LED, HIGH); // Duy trì đèn báo đầy
        delay(400);                  // Tránh quét liên tục khi tay còn ở cảm biến
      } else {
        // Đã được dọn rác -> Bỏ trạng thái đầy
        isTrashFull = false;
        digitalWrite(PIN_LED, LOW);
        sendTelegramMessage("✅ Thùng rác đã được dọn sạch sẽ. Sẵn sàng sử dụng!");
      }
    }

    // Nếu thùng rác không đầy -> Mở nắp bình thường
    if (!isTrashFull) {
      Serial.println(">> [NHAN TIN HIEU TAY] Mario Power-up & mo nap ngay!");
      soundMarioPowerUp();         // Âm thanh Mario ăn nấm siêu vui nhộn
      digitalWrite(PIN_LED, HIGH); // Giữ đèn LED sáng khi nắp đang mở
      moveServoSlow(gocMo);
      isLidOpen = true;
      isPreCloseWarned = false;    // Đặt lại cờ cảnh báo đóng nắp
      lidOpenedTime = millis();    // Bắt đầu tính thời gian mở sau khi nắp đã mở xong
    }
  }

  // Nếu tay vẫn để ở cảm biến khi nắp đang mở thì gia hạn thời gian mở
  if (isLidOpen && irState == LOW) {
    lidOpenedTime = now;
    isPreCloseWarned = false;      // Reset lại cờ cảnh báo nếu được gia hạn thời gian
  }

  // 2. CẢNH BÁO TRƯỚC KHI NẮP HẠ XUỐNG (TRƯỚC 1 GIÂY - ÂM THANH THẢ CỜ MARIO)
  if (isLidOpen && !isPreCloseWarned && (now - lidOpenedTime >= (unsigned long)(thoiGianGiu - CANH_BAO_TRUOC_DONG))) {
    Serial.println(">> [CANH BAO] Nap sap dong sau 1 giay (Mario Flagpole)...");
    soundMarioFlagpole();        // Âm thanh hạ cờ nhẹ nhàng
    digitalWrite(PIN_LED, HIGH); // Giữ đèn sáng tiếp tục
    isPreCloseWarned = true;
  }

  // 3. TỰ ĐỘNG ĐÓNG NẮP VỀ VỊ TRÍ ĐÓNG (20 ĐỘ) - CÓ TÍNH NĂNG CHỐNG KẸP TAY
  if (isLidOpen && (now - lidOpenedTime >= (unsigned long)thoiGianGiu)) {
    Serial.println(">> [HET GIO] Dong nap tu tu ve 20 do...");
    
    // Đóng nắp với cờ kiểm tra vật cản (checkObstacle = true)
    bool closedFully = moveServoSlow(GOC_DONG, true);

    if (!closedFully) {
      // PHÁT HIỆN TAY/VẬT CẢN KHI ĐANG ĐÓNG NẮP -> TỰ ĐỘNG BUNG MỞ LẠI!
      Serial.println(">> [CHONG KEP TAY!] Phat hien vat can -> Tu dong bung nap tro lai!");
      soundMarioPowerUp();         // Phát âm thanh bung nắp vui nhộn
      digitalWrite(PIN_LED, HIGH); // Bật giữ đèn LED sáng
      
      // Bung nắp mở ngược lên góc mở an toàn ngay lập tức
      moveServoSlow(gocMo, false);
      
      // Giữ trạng thái nắp mở và gia hạn lại thời gian mở nắp
      isLidOpen = true;
      isPreCloseWarned = false;
      lidOpenedTime = millis();
      Serial.println(">> Da bung nap an toan! Gia han thoi gian mo nap.");
    } else {
      // Đóng nắp thành công bình thường
      isLidOpen = false;
      isPreCloseWarned = false;

      if (!isTrashFull) {
        digitalWrite(PIN_LED, LOW);
      }
      Serial.println(">> Da dong xong nap thung.");
    }
  }

  // 4. ĐO MỨC RÁC ĐỊNH KỲ MỖI 1 GIÂY (CHỈ ĐO KHI NẮP ĐÓNG)
  if (!isLidOpen && (now - lastSonarTime >= 1000)) {
    lastSonarTime = now;
    float dist = getDistance();
    lastMeasuredDistance = dist;

    if (dist <= mucRacDay && dist > 0.0) {
      if (!isTrashFull) {
        Serial.print(">> [CANH BAO] Thung rac DAY! Khoang cach: ");
        Serial.print(dist, 1);
        Serial.println(" cm");
        isTrashFull = true;
        digitalWrite(PIN_LED, HIGH);

        // Phát âm thanh Mario Game Over hài hước
        soundMarioGameOver();

        // Gửi tin nhắn cảnh báo tới Telegram
        String msg = "⚠️ Thùng rác phòng khách đã đầy (còn " + String(dist, 1) + "cm), bạn nhớ đi đổ rác nhé!";
        sendTelegramMessage(msg);
      }
      digitalWrite(PIN_LED, HIGH);
    } else {
      if (isTrashFull) {
        Serial.println(">> [BINH THUONG] Da do rac -> Tat den.");
        isTrashFull = false;
        digitalWrite(PIN_LED, LOW);

        // Gửi thông báo Telegram khi đã đổ rác
        sendTelegramMessage("✅ Thùng rác đã được dọn sạch sẽ. Sẵn sàng sử dụng!");
      }
    }
  }

  delay(10);
}
