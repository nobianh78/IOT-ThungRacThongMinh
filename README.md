#  Dự Án Thùng Rác Thông Minh IoT (Smart Trash Can ESP32)

Dự án thùng rác thông minh sử dụng vi điều khiển **ESP32**, tích hợp cảm biến hồng ngoại, cảm biến siêu âm, động cơ servo MG995, hệ thống âm thanh 8-bit Mario, cảnh báo rác đầy qua **Telegram Bot**, và giao diện **Web Captive Portal** cho phép cấu hình trực tiếp qua điện thoại.

---

##  Các Tính Năng Nổi Bật

1. ** Mở nắp tự động không chạm:**
   - Cảm biến hồng ngoại (IR) phát hiện tay và mở nắp ngay lập tức.
   - Cơ cấu quay Servo êm ái, mượt mà (điều khiển theo bước `moveServoSlow`).
   - Tự động đóng nắp sau 4 giây (có gia hạn nếu tay vẫn ở cảm biến).

2. ** Chống kẹp tay thông minh (Anti-Pinch / Obstacle Auto-Rebound):**
   - Trong quá trình nắp đóng xuống, nếu cảm biến phát hiện có tay/vật cản, motor lập tức **dừng và bung mở ngược trở lại ngay lập tức**, phát âm thanh cảnh báo an toàn.

3. ** Cảnh báo trước khi nắp đóng (Pre-close Warning):**
   - Trước khi nắp đóng 1 giây, hệ thống phát giai điệu Mario hạ cờ nhẹ nhàng để nhắc nhở người dùng.

4. ** Bảo vệ chống tràn rác (Overfill Protection):**
   - Cảm biến siêu âm HC-SR04 đo mức rác liên tục.
   - Khi thùng rác đầy (khoảng cách < 10cm), hệ thống sẽ **từ chối mở nắp**, phát âm thanh cảnh báo Game Over và bật đèn LED đỏ.

5. ** Bộ âm thanh Mario 8-bit sống động:**
   - **Mở nắp / Bung nắp:** Giai điệu *Mario ăn nấm (Power-Up)* (G4 ➔ B4 ➔ D5 ➔ G5 ➔ B5 ➔ D6).
   - **Sắp đóng nắp:** Giai điệu *Mario hạ cờ (Flagpole)* nhẹ nhàng.
   - **Thùng đầy rác / Từ chối mở:** Giai điệu *Mario Game Over* kinh điển.

6. ** Thông báo qua Telegram:**
   - Tự động gửi tin nhắn báo động về điện thoại khi rác đầy:
     > *" Thùng rác phòng khách đã đầy (còn 8.5cm), bạn nhớ đi đổ rác nhé!"*
   - Tự động gửi thông báo xác nhận khi rác đã được dọn sạch.

7. ** Giao diện Web Captive Portal cấu hình qua WiFi:**
   - ESP32 phát WiFi: **`ThungRac-ThongMinh`** (kết nối bằng điện thoại là tự động hiện trang cấu hình).
   - Quét và chọn mạng WiFi 2.4GHz gia đình.
   - Cài đặt Telegram Bot Token & Chat ID.
   - Tùy chỉnh góc mở nắp, thời gian giữ nắp, khoảng cách báo đầy trực tiếp trên web mà không cần nạp lại code.
   - Toàn bộ cài đặt được lưu vĩnh viễn vào bộ nhớ Flash NVS (`Preferences`).

---

##  Sơ Đồ Đấu Nối Phần Cứng (Pinout)

| Linh Kiện | Chân Linh Kiện | Chân ESP32 | Ghi Chú |
| :--- | :--- | :--- | :--- |
| **Servo MG995** | Signal (Cam/Vàng) | **GPIO 13** | Nguồn 5V riêng hoặc nguồn ngoài |
| **Cảm biến Hồng Ngoại** | OUT | **GPIO 27** | INPUT_PULLUP |
| **Cảm biến Siêu Âm HC-SR04** | TRIG | **GPIO 26** | Kích xung siêu âm |
| **Cảm biến Siêu Âm HC-SR04** | ECHO | **GPIO 25** | Nhận xung phản hồi |
| **Đèn LED Báo Hiệu** | Anode (+) | **GPIO 32** | Nối qua trở 220Ω |
| **Còi Báo (Buzzer)** | VCC / I/O | **GPIO 33** | Còi thụ động/chủ động phát âm |

---

##  Cài Đặt & Nạp Code

Dự án được xây dựng bằng **PlatformIO** (VS Code hoặc PlatformIO Core CLI).

1. Clone kho lưu trữ:
   ```bash
   git clone https://github.com/nobianh78/IOT-ThungRacThongMinh.git
   cd IOT-ThungRacThongMinh
   ```

2. Biên dịch dự án:
   ```bash
   pio run
   ```

3. Nạp code vào ESP32:
   ```bash
   pio run -t upload
   ```

4. Theo dõi Serial Monitor (115200 baud):
   ```bash
   pio device monitor
   ```

---

##  Hướng Dẫn Cấu Hình WiFi & Telegram

1. Sau khi bật nguồn ESP32, mở điện thoại và kết nối vào mạng WiFi: **`ThungRac-ThongMinh`**.
2. Điện thoại sẽ tự động mở trang cấu hình (hoặc truy cập trình duyệt tại địa chỉ `http://192.168.4.1`).
3. Bấm **"Quét mạng WiFi"** ➔ Chọn mạng WiFi gia đình (2.4GHz) ➔ Nhập mật khẩu.
4. Nhập mã **Telegram Bot Token** (lấy từ `@BotFather`) và **Telegram Chat ID** (lấy từ `@userinfobot`).
5. Bấm **` LƯU CẤU HÌNH & KHỞI ĐỘNG LẠI`**. Thùng rác sẽ tự động khởi động lại và kết nối mạng!
