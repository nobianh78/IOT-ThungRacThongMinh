#pragma once
#include <Arduino.h>

const char PAGE_CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Cấu Hình Thùng Rác Thông Minh</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: #0f172a; color: #f8fafc; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 16px; }
    .card { background: #1e293b; border: 1px solid #334155; border-radius: 16px; width: 100%; max-width: 480px; padding: 24px; box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.5); }
    h1 { font-size: 20px; font-weight: 700; color: #38bdf8; text-align: center; margin-bottom: 6px; }
    .subtitle { text-align: center; color: #94a3b8; font-size: 13px; margin-bottom: 18px; }
    .section-title { font-size: 13px; font-weight: 600; color: #38bdf8; margin: 18px 0 10px; display: flex; align-items: center; gap: 6px; text-transform: uppercase; letter-spacing: 0.5px; }
    .form-group { margin-bottom: 12px; }
    label { display: block; font-size: 13px; color: #cbd5e1; margin-bottom: 5px; font-weight: 500; }
    input, select { width: 100%; padding: 10px 12px; background: #0f172a; border: 1px solid #475569; border-radius: 8px; color: #fff; font-size: 14px; outline: none; transition: border-color 0.2s; }
    input:focus, select:focus { border-color: #38bdf8; }
    .row { display: flex; gap: 10px; }
    .row .form-group { flex: 1; }
    .btn { display: inline-flex; justify-content: center; align-items: center; width: 100%; padding: 12px; border-radius: 8px; border: none; font-size: 14px; font-weight: 600; cursor: pointer; transition: all 0.2s; text-decoration: none; }
    .btn-primary { background: linear-gradient(135deg, #0284c7, #0369a1); color: #fff; margin-top: 10px; box-shadow: 0 4px 12px rgba(2, 132, 199, 0.4); }
    .btn-primary:hover { background: linear-gradient(135deg, #0369a1, #075985); }
    .btn-secondary { background: #334155; color: #e2e8f0; font-size: 12px; padding: 7px 12px; width: auto; margin-bottom: 8px; border-radius: 6px; }
    .btn-secondary:hover { background: #475569; }
    .hint { font-size: 11px; color: #64748b; margin-top: 4px; line-height: 1.4; }
    .status-box { background: #0f172a; border: 1px solid #334155; border-radius: 10px; padding: 12px; margin-bottom: 16px; display: grid; grid-template-columns: 1fr 1fr; gap: 10px; font-size: 12px; }
    .status-item { display: flex; flex-direction: column; }
    .status-label { color: #64748b; }
    .status-val { color: #10b981; font-weight: 600; font-size: 13px; margin-top: 2px; }
    #msg { display: none; padding: 12px; border-radius: 8px; margin-top: 14px; text-align: center; font-size: 13px; }
    .msg-success { background: rgba(16, 185, 129, 0.15); border: 1px solid #10b981; color: #34d399; }
  </style>
</head>
<body>
  <div class="card">
    <h1>🗑️ THÙNG RÁC THÔNG MINH</h1>
    <div class="subtitle">Trang Cấu Hình WiFi & Telegram Bot</div>

    <!-- Hộp trạng thái hiện tại -->
    <div class="status-box">
      <div class="status-item">
        <span class="status-label">Trạng thái WiFi:</span>
        <span class="status-val" id="wifi_stat">Đang kiểm tra...</span>
      </div>
      <div class="status-item">
        <span class="status-label">Mức rác (khoảng cách):</span>
        <span class="status-val" id="sonar_dist">-- cm</span>
      </div>
      <div class="status-item">
        <span class="status-label">Trạng thái nắp:</span>
        <span class="status-val" id="lid_stat">--</span>
      </div>
      <div class="status-item">
        <span class="status-label">IP Thiết bị:</span>
        <span class="status-val" id="device_ip">--</span>
      </div>
    </div>

    <form id="configForm">
      <!-- 1. Cấu hình WiFi -->
      <div class="section-title">📶 1. CẤU HÌNH WIFI GIA ĐÌNH</div>
      <button type="button" class="btn btn-secondary" onclick="scanWifi()">🔄 Quét mạng WiFi xung quanh</button>
      
      <div class="form-group" id="scan_container" style="display:none;">
        <label>Chọn mạng đã tìm thấy:</label>
        <select id="wifi_select" onchange="selectWifi(this.value)">
          <option value="">-- Bấm để chọn WiFi --</option>
        </select>
      </div>

      <div class="form-group">
        <label for="ssid">Tên WiFi (SSID):</label>
        <input type="text" id="ssid" name="ssid" placeholder="Nhập hoặc chọn tên WiFi..." required value="%WIFI_SSID%">
        <div class="hint">Lưu ý: ESP32 chỉ hỗ trợ băng tần WiFi 2.4GHz.</div>
      </div>

      <div class="form-group">
        <label for="pass">Mật khẩu WiFi:</label>
        <input type="password" id="pass" name="pass" placeholder="Nhập mật khẩu WiFi..." value="%WIFI_PASS%">
      </div>

      <!-- 2. Cấu hình Telegram -->
      <div class="section-title">💬 2. CẤU HÌNH TELEGRAM BOT</div>
      <div class="form-group">
        <label for="token">Telegram Bot Token:</label>
        <input type="text" id="token" name="token" placeholder="VD: 123456789:ABCdefGhI..." value="%TG_TOKEN%">
        <div class="hint">Lấy mã token miễn phí từ bot <b>@BotFather</b> trên Telegram.</div>
      </div>

      <div class="form-group">
        <label for="chatid">Telegram Chat ID:</label>
        <input type="text" id="chatid" name="chatid" placeholder="VD: 987654321..." value="%TG_CHATID%">
        <div class="hint">Lấy mã ID của bạn từ bot <b>@userinfobot</b> trên Telegram.</div>
      </div>

      <!-- 3. Cấu hình Cơ Khí -->
      <div class="section-title">⚙️ 3. CÀI ĐẶT THÔNG SỐ HOẠT ĐỘNG</div>
      <div class="row">
        <div class="form-group">
          <label for="goc_mo">Góc mở (°):</label>
          <input type="number" id="goc_mo" name="goc_mo" min="60" max="150" value="%GOC_MO%">
        </div>
        <div class="form-group">
          <label for="tg_giu">Giữ nắp (giây):</label>
          <input type="number" id="tg_giu" name="tg_giu" min="2" max="15" value="%TG_GIU%">
        </div>
        <div class="form-group">
          <label for="muc_day">Báo đầy (cm):</label>
          <input type="number" id="muc_day" name="muc_day" min="5" max="30" value="%MUC_DAY%">
        </div>
      </div>

      <button type="submit" class="btn btn-primary">💾 LƯU CẤU HÌNH & KHỞI ĐỘNG LẠI</button>
    </form>

    <div id="msg"></div>
  </div>

  <script>
    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(d => {
          document.getElementById('wifi_stat').innerText = d.wifi_connected ? 'Đã kết nối (' + d.ssid + ')' : 'Chưa kết nối';
          document.getElementById('wifi_stat').style.color = d.wifi_connected ? '#34d399' : '#f87171';
          document.getElementById('sonar_dist').innerText = (d.distance > 0 && d.distance < 900) ? (d.distance + ' cm') : 'Không xác định';
          document.getElementById('lid_stat').innerText = d.lid_open ? 'Đang MỞ' : 'Đang ĐÓNG';
          document.getElementById('device_ip').innerText = d.ip;
        })
        .catch(() => {});
    }
    setInterval(updateStatus, 1500);
    updateStatus();

    function scanWifi() {
      const btn = event.target;
      btn.innerText = '⏳ Đang quét...';
      fetch('/scan')
        .then(r => r.json())
        .then(list => {
          btn.innerText = '🔄 Quét lại mạng';
          const sel = document.getElementById('wifi_select');
          sel.innerHTML = '<option value="">-- Bấm để chọn WiFi --</option>';
          list.forEach(item => {
            const opt = document.createElement('option');
            opt.value = item.ssid;
            opt.innerText = item.ssid + ' (' + item.rssi + ' dBm)';
            sel.appendChild(opt);
          });
          document.getElementById('scan_container').style.display = 'block';
        })
        .catch(e => {
          btn.innerText = '🔄 Quét lại mạng';
          alert('Không thể quét WiFi!');
        });
    }

    function selectWifi(val) {
      if (val) document.getElementById('ssid').value = val;
    }

    document.getElementById('configForm').onsubmit = function(e) {
      e.preventDefault();
      const btn = document.querySelector('.btn-primary');
      btn.disabled = true;
      btn.innerText = '⏳ Đang lưu cấu hình...';

      const formData = new FormData(this);
      fetch('/save', { method: 'POST', body: formData })
        .then(r => r.text())
        .then(t => {
          const msg = document.getElementById('msg');
          msg.className = 'msg-success';
          msg.innerHTML = '✅ <b>Lưu thành công!</b><br>ESP32 đang khởi động lại để áp dụng cài đặt mới...';
          msg.style.display = 'block';
          btn.innerText = 'Đã lưu!';
        })
        .catch(err => {
          alert('Có lỗi xảy ra khi lưu: ' + err);
          btn.disabled = false;
          btn.innerText = '💾 LƯU CẤU HÌNH & KHỞI ĐỘNG LẠI';
        });
    };
  </script>
</body>
</html>
)rawliteral";

