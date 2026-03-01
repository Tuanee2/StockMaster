# Project Overview

## 1) Mục tiêu

StockMaster là ứng dụng quản lý bán buôn theo hướng desktop (Qt/QML), tập trung vào:
- quản lý khách hàng
- quản lý sản phẩm
- quản lý lô và tồn kho theo lô
- theo dõi snapshot tồn và lịch sử biến động tồn
- điều chỉnh tồn có lý do
- theo dõi hạn dùng lô và cảnh báo sắp hết hạn
- quản lý đơn hàng draft/confirm/void
- quản lý phiếu thu và công nợ theo khách
- báo cáo tổng hợp và export file
- kiểm tra GitHub Release mới và tải gói cập nhật từ màn Settings

## 2) Stack đang dùng

- UI: Qt Quick / QML
- Logic: C++17
- Build: CMake + Qt6
- Release: GitHub Actions build/package cho macOS + Windows
- DB layer: SQLite local-first qua `DatabaseService` (đã mở file DB thật + bootstrap schema)

## 3) Trạng thái triển khai hiện tại

Đã chạy được:
- Dashboard mixed mode:
  - KPI vận hành
  - công nợ phải thu
  - cảnh báo vận hành
- biểu đồ doanh số vs thu tiền 7 ngày gần nhất
  - top khách hàng theo doanh số
  - top sản phẩm bán chạy
- Reports flow:
  - sales summary theo kỳ
  - debt aging report
  - inventory movement report
  - export CSV/PDF
- Customer CRUD
- Product CRUD
- SKU tự sinh khi tạo mới
- Lot management theo product
- thêm hạn dùng cho từng lô
- Stock in/out theo lô
- Inventory flow:
  - snapshot tồn hiện tại theo mặt hàng
  - snapshot tồn theo lô
  - lịch sử biến động tồn theo mặt hàng
  - cảnh báo tồn thấp (<= 20)
  - điều chỉnh tồn có lý do
- cảnh báo mặt hàng có lô sắp hết hạn (<= 30 ngày)
- Order draft lifecycle:
  - tạo draft
  - sửa khách hàng của draft
  - thêm/cập nhật/xóa item
  - chọn lô ưu tiên + tự phân bổ qua nhiều lô nếu cần
  - confirm (trừ kho theo lô)
  - void (hoàn kho)
- Query danh sách đơn:
  - theo mã đơn
  - theo ngày cụ thể
  - theo khoảng ngày
- Payment flow:
  - lập phiếu thu
  - chặn over-payment
  - cập nhật trạng thái `PartiallyPaid` / `Paid`
  - đối soát ledger theo khách
- Settings updater:
  - kiểm tra GitHub Release theo thao tác người dùng
  - chỉ tải gói cập nhật đúng nền tảng khi người dùng bấm `Cập nhật`
  - tự mở gói vừa tải sau khi tải xong
  - giữ nguyên DB cũ vì SQLite nằm ngoài thư mục cài đặt
- Windows release hiện được đóng gói thành installer `.exe` thật, không còn chỉ là file nén `.zip`

Chưa triển khai đầy đủ:
- debt ledger hiện vẫn là đối soát suy diễn từ order/payment, chưa lưu bảng riêng
- report hiện vẫn là tổng hợp từ cache service, chưa có repository/report query tối ưu riêng
- settings hiện mới là updater tối giản, chưa có backup/restore hoặc cấu hình sync
- sync

## 4) Kiến trúc thực thi hiện tại

- `QML` chỉ hiển thị + điều hướng
- `ViewModel` (QObject/QAbstractListModel) làm state/action cho UI
- `Application Service` xử lý nghiệp vụ và kiểm tra dữ liệu
- `Domain` là cấu trúc dữ liệu nghiệp vụ

Lưu ý:
- `CustomerService`, `ProductService`, `OrderService`, `PaymentService` hiện load cache từ SQLite khi khởi tạo.
- Thao tác CRUD/nghiệp vụ sẽ ghi lại snapshot bảng vào file SQLite local.
- Nếu DB chưa sẵn sàng, service vẫn fallback chạy in-memory để app không chết cứng.
