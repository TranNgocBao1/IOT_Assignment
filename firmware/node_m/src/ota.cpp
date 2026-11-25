// =============================================================================
// ota.cpp — Module M6, phần của Bạn A (client OTA trên board)
//
// Luồng OTA của ThingsBoard qua MQTT:
//   1. Người dùng gán gói firmware trên CoreIOT -> server đẩy shared attributes
//      fw_title, fw_version, fw_size, fw_checksum, fw_checksum_algorithm.
//   2. Thiết bị so sánh với FW_TITLE / FW_VERSION đang chạy. Khác version -> tải.
//   3. Xin từng chunk: publish "v2/fw/request/{reqId}/chunk/{i}", payload = kích
//      thước chunk (chuỗi số, vd "4096"). Server trả binary ở
//      "v2/fw/response/{reqId}/chunk/{i}".
//   4. Ghi chunk vào slot OTA rảnh (Update.write) và cập nhật hash.
//   5. Đủ fw_size byte -> so checksum -> Update.end(true) -> khởi động lại.
//   6. Sau khi boot bản mới: báo current_fw_version và fw_state = UPDATED.
//
// Thuật toán dùng trong file:
//  - Máy trạng thái (finite state machine) IDLE -> DOWNLOADING -> DOWNLOADED ->
//    VERIFIED -> UPDATING -> (reboot) -> UPDATED, lỗi ở bất kỳ bước nào -> FAILED.
//  - Tải theo khối (chunked transfer) theo kiểu dừng-và-chờ (stop-and-wait):
//    chỉ xin chunk i+1 khi đã ghi xong chunk i => không cần bộ nhớ lớn.
//  - Băm tăng dần SHA-256 (incremental hashing) bằng mbedtls: update() mỗi chunk,
//    finish() khi đủ, so sánh chuỗi hex với fw_checksum. Sai -> Update.abort(),
//    bản cũ vẫn chạy bình thường (nhờ 2 slot OTA).
//  - Timeout + thử lại: quá OTA_CHUNK_TIMEOUT_MS không có chunk thì xin lại,
//    quá OTA_MAX_RETRY lần thì FAILED.
// =============================================================================
#include <Update.h>
#include <mbedtls/sha256.h>
#include "ota.h"
#include "config.h"
#include "coreiot.h"

static OtaState s_state = OtaState::IDLE;

// Thông tin gói đang tải
static String   s_targetTitle, s_targetVersion, s_checksum, s_algo;
static uint32_t s_fwSize       = 0;
static uint32_t s_received     = 0;
static int      s_nextChunk    = 0;
static uint32_t s_requestId    = 1;
static uint32_t s_lastChunkMs  = 0;
static int      s_retry        = 0;
static mbedtls_sha256_context s_sha;

static void verifyAndApply();   // khai báo trước, định nghĩa ở dưới otaOnChunk()

static const char* stateName(OtaState s) {
  switch (s) {
    case OtaState::DOWNLOADING: return "DOWNLOADING";
    case OtaState::DOWNLOADED:  return "DOWNLOADED";
    case OtaState::VERIFIED:    return "VERIFIED";
    case OtaState::UPDATING:    return "UPDATING";
    case OtaState::UPDATED:     return "UPDATED";
    case OtaState::FAILED:      return "FAILED";
    default:                    return "IDLE";
  }
}

// Đổi trạng thái và báo lên CoreIOT (telemetry fw_state)
static void setState(OtaState s, const char* error = nullptr) {
  s_state = s;
  Serial.printf("[OTA] fw_state = %s\n", stateName(s));
  if (s == OtaState::IDLE) return;
  coreiotSendTelemetryString("fw_state", stateName(s));
  if (error) coreiotSendTelemetryString("fw_error", error);
}

static void requestChunk(int index) {
  // TODO[M6-A]:
  //   char topic[64];
  //   snprintf(topic, sizeof(topic), "v2/fw/request/%lu/chunk/%d", s_requestId, index);
  //   char size[8]; snprintf(size, sizeof(size), "%d", OTA_CHUNK_SIZE);
  //   coreiotPublish(topic, size);
  s_lastChunkMs = millis();
}

static void fail(const char* why) {
  Serial.printf("[OTA] LOI: %s\n", why);
  Update.abort();
  mbedtls_sha256_free(&s_sha);
  setState(OtaState::FAILED, why);
}

// Chuyển 32 byte hash thành chuỗi hex 64 ký tự thường để so với fw_checksum
static String toHex(const uint8_t* b, size_t n) {
  static const char* hx = "0123456789abcdef";
  String s; s.reserve(n * 2);
  for (size_t i = 0; i < n; i++) { s += hx[b[i] >> 4]; s += hx[b[i] & 0xF]; }
  return s;
}

// ----------------------------------------------------------------------------
void otaBegin() {
  Serial.printf("[OTA] dang chay %s %s\n", FW_TITLE, FW_VERSION);
}

void otaOnConnected() {
  // Báo phiên bản đang chạy để CoreIOT hiển thị đúng
  // TODO[M6-A]: gửi telemetry current_fw_title = FW_TITLE, current_fw_version = FW_VERSION
}

void otaOnFirmwareInfo(JsonObjectConst shared) {
  const char* title   = shared["fw_title"]   | "";
  const char* version = shared["fw_version"] | "";
  if (strlen(title) == 0 || strlen(version) == 0) return;

  // Trường hợp 1: vừa boot xong bản mới => bản đích chính là bản đang chạy
  if (strcmp(title, FW_TITLE) == 0 && strcmp(version, FW_VERSION) == 0) {
    // TODO[M6-A]: nếu chưa báo thì setState(OtaState::UPDATED)
    return;
  }
  // Trường hợp 2: sai tên firmware (gói của thiết bị khác) => bỏ qua
  if (strcmp(title, FW_TITLE) != 0) {
    Serial.printf("[OTA] bo qua goi '%s' (khong phai %s)\n", title, FW_TITLE);
    return;
  }
  // Đang tải dở thì không bắt đầu lại
  if (s_state == OtaState::DOWNLOADING) return;

  // Trường hợp 3: có phiên bản mới => bắt đầu tải
  s_targetTitle   = title;
  s_targetVersion = version;
  s_fwSize        = shared["fw_size"] | 0;
  s_checksum      = shared["fw_checksum"] | "";
  s_algo          = shared["fw_checksum_algorithm"] | "SHA256";
  Serial.printf("[OTA] co ban moi %s, %u byte, %s\n", version, s_fwSize, s_algo.c_str());

  // TODO[M6-A]:
  //  1. Kiểm tra s_fwSize > 0 và s_algo là "SHA256" (nếu chọn MD5 trên CoreIOT thì
  //     đổi sang mbedtls_md5 tương ứng). Sai -> fail("...") và return.
  //  2. if (!Update.begin(s_fwSize)) { fail("khong du cho trong slot OTA"); return; }
  //  3. mbedtls_sha256_init(&s_sha); mbedtls_sha256_starts(&s_sha, 0);  // 0 = SHA-256
  //  4. s_received = 0; s_nextChunk = 0; s_retry = 0; s_requestId++;
  //  5. setState(OtaState::DOWNLOADING); requestChunk(0);
}

void otaOnChunk(int chunkIndex, const uint8_t* data, size_t len) {
  if (s_state != OtaState::DOWNLOADING) return;
  if (chunkIndex != s_nextChunk) return;   // chunk cũ / trùng -> bỏ

  // TODO[M6-A]:
  //  1. if (Update.write((uint8_t*)data, len) != len) { fail("ghi flash loi"); return; }
  //  2. mbedtls_sha256_update(&s_sha, data, len);
  //  3. s_received += len; s_nextChunk++; s_retry = 0;
  //  4. In tiến độ mỗi ~10%: s_received * 100 / s_fwSize
  //  5. Nếu s_received < s_fwSize: requestChunk(s_nextChunk);
  //     Ngược lại: setState(DOWNLOADED) rồi gọi verifyAndApply() (viết bên dưới).
}

// Kiểm tra checksum và áp dụng firmware mới
static void verifyAndApply() {
  // TODO[M6-A]:
  //  1. uint8_t hash[32]; mbedtls_sha256_finish(&s_sha, hash); mbedtls_sha256_free(&s_sha);
  //  2. so toHex(hash, 32) với s_checksum (so không phân biệt hoa thường: equalsIgnoreCase)
  //     sai -> fail("checksum sai"); return;
  //  3. setState(VERIFIED);
  //  4. if (!Update.end(true)) { fail(Update.errorString()); return; }
  //  5. setState(UPDATING); delay(500) cho MQTT kịp gửi; ESP.restart();
}

void otaLoop() {
  if (s_state != OtaState::DOWNLOADING) return;
  // TODO[M6-A]: nếu millis() - s_lastChunkMs > OTA_CHUNK_TIMEOUT_MS:
  //   s_retry++; nếu s_retry > OTA_MAX_RETRY -> fail("timeout"); ngược lại requestChunk(s_nextChunk)
  // Gợi ý: nếu MQTT mất kết nối giữa chừng thì cũng fail để lần sau tải lại từ đầu.
  (void)verifyAndApply;  // xóa dòng này khi đã gọi verifyAndApply() trong otaOnChunk
}

OtaState otaState() { return s_state; }
