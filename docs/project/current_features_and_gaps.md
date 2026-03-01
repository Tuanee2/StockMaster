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
- Smoke test:
  - `report_service_smoke`
  - `dashboard_view_model_smoke`
  - `inventory_service_smoke`
  - `payment_service_smoke`

## 2) Feature mới thêm gần nhất (Reports)

- Thay `ReportsScreen` placeholder bằng flow thật
- Thêm `ReportService`
- Thêm `ReportsViewModel`
- Thêm export `CSV/PDF` cho từng nhóm báo cáo
- Nút `Làm mới` ở shell sẽ refresh thêm `Reports` khi đang đứng ở tab này

## 3) Giới hạn kỹ thuật hiện tại

- Dữ liệu in-memory, chưa persistence thật qua SQLite tables.
- `DatabaseService` mới là bootstrap placeholder.
- Payment/debt hiện mới chạy ở mức in-memory, chưa lưu xuống DB.
- Inventory movement cũng mới ở mức in-memory.
- Dashboard chart, ranking và report hiện đều là tổng hợp in-memory từ dữ liệu hiện có.
- Report export PDF hiện là bản PDF text-based đơn giản, chưa có layout in ấn nâng cao.

## 4) Đề xuất thứ tự triển khai tiếp theo

1. Hoàn thiện `infra/db` với schema + repository + migration.
2. Chuyển `CustomerService/ProductService/OrderService` từ in-memory sang repository.
3. Persistence `PaymentService` + ledger + inventory movement xuống SQLite.
4. Bổ sung test cho:
   - confirm/void order
   - stock in/out + inventory adjustment edge cases
   - order query by date/range
   - apply payment / over-payment
5. Hoàn thiện các screen Reports/Settings.
