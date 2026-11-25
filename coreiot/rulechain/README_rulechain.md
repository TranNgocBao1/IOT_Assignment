# RuleChain cho Node M (module M5)

Dựng trong CoreIOT: **Rule chains → + → tên `air-quality`**, gán làm rule chain của Device profile chứa `air-node-m`.

```
[Input]
   │
[Message type switch] ── Post telemetry ──► [Originator attributes]  (Shared: co2_on, co2_off, co2_alarm)
                                                   │
                         ┌─────────────────────────┼──────────────────────────┐
                         ▼                         ▼                          ▼
              [Script: 01_filter_fan_on]  [Script: 02_filter_fan_off]  [Script: 04_filter_alarm]
                    │ True                       │ True                    │ True        │ False
                    ▼                            ▼                         ▼             ▼
       [Transform: 03 (params:true)]  [Transform: 03 (params:false)]  [Create alarm] [Clear alarm]
                    │                            │                         │
                    ▼                            ▼                         ▼
           [RPC call request]           [RPC call request]        (Notification / email)
   (nhánh Post telemetry cũng phải nối tới [Save timeseries] như rule chain mặc định)
```

Lưu ý:
- **Giữ node `Save timeseries`** của rule chain mặc định, nếu không dashboard sẽ trống.
- Trong node `Originator attributes`, các key shared được đưa vào metadata với tiền tố `shared_` (vd. `shared_co2_on`). Kiểm tra bằng nút **Debug** của node.
- Test bằng `tools/fake_telemetry.py` trước, sau đó mới test với cảm biến thật.
- Chụp ảnh rule chain và màn hình alarm để đưa vào báo cáo.
