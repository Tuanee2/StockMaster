# Current Features And Gaps

## 1) Feature đã hoàn thành và chạy được

- Customer CRUD (mã khách tự sinh)
- Product CRUD
- Quản lý lô sản phẩm
  - có ngày hết hạn cho từng lô
- Nhập/xuất tồn theo lô
- Inventory flow:
  - snapshot tồn hiện tại theo mặt hàng
  - snapshot theo lô
  - lịch sử biến động tồn
  - cảnh báo tồn thấp (<= 20)
  - điều chỉnh tồn có lý do
- Cảnh báo lô sắp hết hạn:
  - mức cảnh báo khi còn <= 30 ngày
  - hiển thị theo sản phẩm và ở dashboard
- Order draft lifecycle:
  - tạo draft
  - cập nhật khách
  - thêm/cập nhật/xóa item
  - confirm với trừ kho theo allocation
  - void với hoàn kho
- Query đơn theo mã hoặc ngày/khoảng ngày
- Dashboard metrics cơ bản
- Dashboard mixed mode:
  - KPI vận hành
  - cảnh báo vận hành
  - biểu đồ doanh số vs thu tiền 6 tháng
  - top khách hàng
  - top sản phẩm
- Reports flow:
  - sales summary theo kỳ
  - debt aging report
  - inventory movement report
  - export CSV/PDF
- Payment flow:
  - lập phiếu thu
  - chống over-payment
  - tự cập nhật trạng thái `PartiallyPaid` / `Paid`
  - đối soát ledger theo khách
- SQLite local persistence:
  - customer / product / lot / inventory movement
  - order / order item / stock allocation
  - payment
- Settings updater:
  - khi vào tab sẽ tự kiểm tra GitHub Release mới nhất
  - có 1 nút để kiểm tra lại và tải gói cập nhật
  - tự tải gói cập nhật theo nền tảng và tự mở gói sau khi tải xong
  - không đụng DB local
- GitHub Actions release packaging:
  - macOS `.dmg`
  - Windows installer `.exe` qua `windeployqt` + `NSIS`
- Smoke test:
  - `report_service_smoke`
  - `sqlite_persistence_smoke`
  - `dashboard_view_model_smoke`
  - `inventory_service_smoke`
  - `payment_service_smoke`

## 2) Feature mới thêm gần nhất (Release updater + packaging)

- Thêm `SettingsViewModel` để:
  - kiểm tra `releases/latest` trên GitHub
  - so sánh version hiện tại với `tag_name`
  - tự tải gói update về thư mục tải xuống
- `SettingsScreen` không còn là placeholder; hiện chỉ giữ 1 nút cập nhật và phần trạng thái tải
- Thêm workflow `.github/workflows/release.yml`:
  - build `Release` trên macOS và Windows
  - dùng `macdeployqt` / `windeployqt`
  - Windows gọi thêm `NSIS` để tạo installer `.exe`
  - upload asset vào GitHub Release khi push tag `v*`

## 3) Giới hạn kỹ thuật hiện tại

- Service layer vẫn giữ cache in-memory để phục vụ UI, nhưng đã có persistence thật xuống SQLite.
- `DebtLedger` vẫn là dữ liệu suy diễn, chưa có bảng lưu riêng.
- Dashboard chart, ranking và report hiện đều là tổng hợp in-memory từ dữ liệu hiện có.
- Report export PDF hiện là bản PDF text-based đơn giản, chưa có layout in ấn nâng cao.
- Chưa có DB worker thread riêng; hiện query/ghi vẫn chạy đồng bộ trên luồng gọi.
- Chưa có migration versioned nhiều bước, mới ở mức bootstrap schema version 1.
- Updater đã có bước tự mở gói sau khi tải, nhưng vẫn phụ thuộc shell mặc định của hệ điều hành và chưa có updater helper riêng.

## 4) Đề xuất thứ tự triển khai tiếp theo

1. Tách dần persistence từ snapshot rewrite sang repository/query theo bảng để giảm chi phí ghi toàn bảng.
2. Thêm bảng `debt_ledger` thật nếu muốn đối soát lịch sử độc lập với dữ liệu suy diễn.
3. Đưa DB access sang worker thread hoặc connection riêng theo thread để tránh block UI khi dữ liệu lớn.
4. Bổ sung test cho:
   - confirm/void order
   - stock in/out + inventory adjustment edge cases
   - order query by date/range
   - apply payment / over-payment
5. Nếu cần trải nghiệm update liền mạch hơn, thêm bước mở gói cài đặt hoặc updater helper riêng sau khi tải xong.
