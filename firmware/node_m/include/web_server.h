// =============================================================================
// web_server.h — Web server chạy ngay trên Node M (module M3, T07)
// Dùng được khi mất internet, chỉ cần chung mạng LAN.
// Gateway Python (M7) cũng gọi các API này.
// =============================================================================
#pragma once

void webBegin();
void webLoop();   // gọi trong loop() — WebServer đồng bộ cần handleClient()
