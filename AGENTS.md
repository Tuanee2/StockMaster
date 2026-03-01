# AGENTS.md — AI Control Panel (StockMaster App)

> **Entry point cho Codex/agent.** AI PHẢI đọc file này TRƯỚC KHI đọc bất kỳ thứ gì khác trong repo.

## Project Overview

- **Tên**: StockMaster
- **Mục tiêu**: Xây ứng dụng **quản lý bán buôn** (desktop + mobile) hỗ trợ **khách hàng, sản phẩm, kho, đơn hàng, công nợ, thanh toán, báo cáo** và đồng bộ dữ liệu.
- **Tech stack dự kiến**:
  - UI: **Qt / QML**
  - Local DB: **SQLite**
  - Sync (tùy chọn): **Supabase** (đồng bộ hai chiều / multi-device)
- **Ngôn ngữ tài liệu**: Tiếng Việt (code + API dùng tiếng Anh).

## Thứ tự đọc bắt buộc (Reading Order)

1. `AGENTS.md` (file này) — rules/constraints/workflow
2. `docs/_index.md` — bản đồ tài liệu (AI phải follow để chỉ load đúng file cần thiết)
3. Các file docs được `_index.md` trỏ tới (architecture/data-model/db/sync/ui/flows)
4. `src/` (source code) — **code là source of truth**

> **Nguyên tắc**: Khi document và code mâu thuẫn → tin code.

## Rules (BẮT BUỘC)

### 1) Explicit > Implicit
- Không đoán. Nếu requirement mơ hồ → nêu giả định rõ ràng trong PR/commit message hoặc hỏi lại.
- Không tự ý thêm feature ngoài scope task.

### 2) Product rules (Wholesale domain)
- **Số liệu tài chính phải chính xác**:
  - Tiền tệ dùng **số nguyên** (VD: VND) hoặc kiểu **decimal** an toàn (không dùng `double` cho tiền).
  - Rounding/đơn vị phải nhất quán (VND không có phần thập phân).
- **Bất biến nghiệp vụ** (không được phá):
  - Đơn hàng có trạng thái rõ ràng (draft/confirmed/paid/void… tùy thiết kế).
  - Tồn kho không âm (trừ khi có cơ chế cho phép âm và ghi rõ).
  - Công nợ phát sinh từ đơn và thanh toán, có thể đối soát theo khách hàng.

### 3) Performance + UX constraints
- UI thread **không được block** khi truy vấn DB/sync mạng.
- Mọi tác vụ nặng (sync, import/export, báo cáo lớn) chạy background và có progress/cancel.
- Danh sách lớn (hàng nghìn bản ghi) phải dùng model hiệu quả (ListView/TableView + paging/lazy load khi cần).

### 4) Data integrity & concurrency
- SQLite truy cập theo nguyên tắc thread-safety:
  - Ưu tiên **1 thread DB** (worker) + queue request, hoặc connection tách theo thread (đúng Qt).
  - Transaction cho các thao tác ghi liên quan (VD: tạo đơn + chi tiết + cập nhật tồn kho).
- Tránh race condition giữa local edit và sync:
  - Có cơ chế versioning/updatedAt/etag hoặc conflict strategy (last-write-wins hoặc merge rule rõ ràng).

### 5) Security & privacy
- Không log dữ liệu nhạy cảm (token, thông tin khách hàng riêng tư) vào log public.
- Thông tin kết nối/sync (API keys, URL) phải cấu hình qua file cấu hình hoặc biến môi trường; tránh hardcode.

### 6) API/UI architecture rules
- Phân lớp rõ ràng:
  - **UI (QML)**: hiển thị, binding, navigation.
  - **ViewModel/Controller (QObject + Q_PROPERTY)**: state, actions, validation, formatting.
  - **Services**: database, sync, report, export/import.
  - **Domain models**: cấu trúc dữ liệu và rule nghiệp vụ.
- Không expose cấu trúc quá phức tạp sang QML (ưu tiên DTO đơn giản, list model chuẩn Qt).

### 7) Coding style
- C++: modern C++ (C++17 trở lên), rõ ràng, ít macro.
- Tên lớp: `PascalCase`, hàm/biến: `camelCase` hoặc `lower_snake_case` nhưng **nhất quán** trong repo.
- Comment giải thích **WHY**, không giải thích WHAT.

### 8) Build / tests
- Mọi thay đổi phải giữ build chạy.
- Khi thêm logic nghiệp vụ (công nợ, tồn kho, giá, thuế):
  - kèm **unit test** tối thiểu cho các rule quan trọng.
- Khi thay đổi schema DB:
  - cập nhật migration + test migrate/rollback (nếu có).

## Workflow bắt buộc cho agent (Codex-friendly)

### A) Trước khi code
1. Đọc `docs/_index.md` để chọn đúng file docs cần đọc.
2. Tóm tắt lại requirement + quyết định kiến trúc (ngắn gọn) trong PR description hoặc commit body.

### B) Khi code
- Đi từ skeleton → chạy được demo → thêm flow nghiệp vụ → tối ưu.
- Ưu tiên correctness + data integrity trước, rồi mới tối ưu UX/perf.

### C) Khi thiếu thông tin
- Nếu thiếu thông số quan trọng (đơn vị tiền, VAT/thuế, loại giá sỉ/lẻ, chính sách tồn kho âm, workflow công nợ):
  - đưa ra default hợp lý **và ghi rõ trong docs/commit**.

## Default assumptions (nếu user chưa cung cấp)

- Target: Qt 6.x (ưu tiên), vẫn giữ khả năng backport nếu cần.
- Local-first: mọi thao tác cơ bản chạy offline với SQLite.
- Sync (nếu bật): best-effort + retry; ưu tiên không làm mất dữ liệu.
- Tiền tệ mặc định: VND (integer).

## Deliverables checklist

- [ ] Data model: Customer / Product / Inventory / Order / OrderItem / Payment / DebtLedger
- [ ] SQLite schema + migration
- [ ] Service layer: DatabaseService + ReportService + Export/Import
- [ ] Sync layer (tùy chọn): Supabase client + conflict policy
- [ ] UI flows: danh sách + tạo/sửa + tìm kiếm + lọc + chi tiết
- [ ] Báo cáo cơ bản: doanh thu, công nợ, tồn kho
- [ ] Demo data + smoke tests
- [ ] Docs cập nhật (docs/*) khi thay đổi kiến trúc/API
- [ ] Project document (doc*/project/*) được cập nhật sau mỗi lần thay đổi và build thành công