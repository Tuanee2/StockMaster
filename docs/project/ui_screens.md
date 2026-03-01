# UI Screens

## 1) Main và App Shell

- `Main.qml`: `ApplicationWindow` + mount `AppShell`.
- `AppShell.qml`:
  - sidebar section list hiển thị tiếng Việt:
    - Tổng quan
    - Khách hàng
    - Sản phẩm
    - Đơn hàng
    - Tồn kho
    - Thanh toán
    - Báo cáo
    - Cài đặt
  - khóa section nội bộ vẫn giữ tiếng Anh để không làm vỡ logic điều hướng
  - header hiển thị tên section đã localize + nút "Làm mới"
  - có thêm nút `Thoát chương trình` trong sidebar, dùng màu cảnh báo đỏ để dễ nhận biết
  - khi nhấn nút thoát sẽ mở hộp thoại xác nhận gọn:
    - có nút `Hủy`
    - có nút `Thoát`
    - có thể đóng bằng `Esc` hoặc click ra ngoài để hủy thao tác thoát
  - `StackLayout` chuyển màn theo `appViewModel.currentSection`

## 2) DashboardScreen

- Tầng 1: vận hành
  - 5 KPI cards:
    - Khách hàng
    - Sản phẩm
    - Đơn mở
    - Sắp hết hàng
    - Mặt hàng sắp hết hạn lô (<= 30 ngày)
  - card `Công nợ phải thu`
    - là card nhấn mạnh, được tăng visual weight
    - khi ở layout 2 cột sẽ cân chiều cao với `Cảnh báo vận hành`
  - card `Cảnh báo vận hành`:
    - cảnh báo công nợ
    - cảnh báo tồn thấp
    - cảnh báo lô sắp hết hạn
- Tầng 2: phân tích
  - biểu đồ cột đôi `Doanh số và thu tiền 6 tháng`
  - `Top khách hàng theo doanh số`
  - `Top sản phẩm bán chạy`

## 3) CustomersScreen

- Bên trái:
  - tìm kiếm khách
  - list khách
- Bên phải:
  - form thêm/sửa/xóa khách
  - mã khách chỉ hiển thị, không nhập tay (tự sinh khi lưu)
  - vùng báo lỗi `lastError`

## 4) ProductsScreen

- Bên trái:
  - tìm kiếm sản phẩm
  - list sản phẩm + tồn tổng + số lô
- Bên phải:
  - form CRUD sản phẩm
  - khối quản lý lô cho sản phẩm đang chọn
    - có `Ngày nhập` và `Hạn dùng` khi thêm lô
    - cảnh báo số lô sắp hết hạn trong 30 ngày
    - badge trạng thái `Quá hạn` hoặc `Còn N ngày` trên từng lô
  - khối nhập/xuất theo lô đang chọn
  - vùng báo lỗi `lastError`

UI đã được chỉnh:
- action button màu solid (không trong suốt)
- field sử dụng layout fill/min width để hạn chế tràn khi thu nhỏ

## 5) OrdersScreen

Màn hình chia 2 cột rõ ràng:

### Cột trái: Danh sách đơn
- Header "Danh sách đơn" + tổng đơn.
- Khối 1: "Tùy chọn lọc danh sách đơn"
  - lọc theo mã đơn
  - lọc theo từ ngày/đến ngày
  - nút `Lọc`, `Xóa lọc`
- Khối 2: "Danh sách đơn theo bộ lọc"
  - list đơn kết quả sau lọc

### Cột phải: Điều hướng nghiệp vụ Order
- Thanh tab nhỏ:
  - `Tạo Draft`
  - `Chi tiết đơn`
  - `Item & Xử lý`
- Tab `Tạo Draft`: tạo draft mới.
- Tab `Chi tiết đơn`: xem và đổi khách hàng của draft.
- Tab `Item & Xử lý`:
  - thêm/cập nhật/xóa item
  - confirm (trừ kho)
  - void (hoàn kho)
- Có `lastError` banner hiển thị lỗi nghiệp vụ.

UI đã được chỉnh:
- textfield/combo custom solid background
- màu chữ và placeholder tương phản nền sáng
- layout ngang, không dùng bố cục trên/dưới như bản cũ

## 6) PaymentsScreen

- Bên trái:
  - tìm kiếm khách theo mã/tên
  - list khách với công nợ hiện tại, số đơn chờ thu, số phiếu thu
- Bên phải:
  - summary công nợ của khách đang chọn
  - form lập phiếu thu
    - chọn đơn còn công nợ
    - nhập số tiền
    - nhập phương thức
    - nhập ngày thu
  - khối lịch sử phiếu thu
  - khối ledger đối soát
  - vùng báo lỗi `lastError`

Layout note:
- card `Công nợ hiện tại` và `Lập phiếu thu` tự tính chiều cao theo nội dung để không bị khối lịch sử/ledger đè lên khi layout co giãn.

Rule UI đang phản ánh:
- không cho lập phiếu thu nếu khách không có đơn chờ thu
- nhắc rõ phiếu thu tự chặn over-payment
- sau khi thu tiền, dashboard và công nợ cập nhật lại

## 7) InventoryScreen

- Bên trái:
  - thanh tìm kiếm snapshot theo SKU/tên/đơn vị
  - summary số mặt hàng đang hiển thị
  - summary số mặt hàng tồn thấp (<= 20)
  - list snapshot tồn hiện tại theo mặt hàng
- Bên phải:
  - summary tồn tổng của mặt hàng đang chọn
  - cảnh báo tồn thấp theo mặt hàng
  - form `Điều chỉnh tồn có lý do`
    - chọn lô
    - nhập số lượng cộng/trừ
    - nhập lý do
    - nhập ngày điều chỉnh
  - khối `Snapshot theo lô`
  - khối `Lịch sử biến động`
  - vùng báo lỗi `lastError`

Rule UI đang phản ánh:
- chỉ cho điều chỉnh khi đã chọn sản phẩm và có lô
- chỉnh tồn thành công sẽ refresh lại dashboard và màn Products
- list snapshot highlight rõ mặt hàng tồn thấp

## 8) SettingsScreen

- Không còn là placeholder.
- Mục tiêu hiện tại: chỉ làm updater tối giản cho desktop release.
- Khi tab được mở:
  - tự kiểm tra GitHub Release mới nhất
  - nếu có bản mới đúng nền tảng thì tự tải gói cập nhật
  - sau khi tải xong sẽ tự mở gói vừa tải
- Trong phần nội dung chỉ giữ 1 nút thao tác:
  - `Kiểm tra và tải bản cập nhật`
- Hiển thị:
  - version hiện tại
  - version mới nhất trên release
  - trạng thái kiểm tra/tải
  - progress tải
  - đường dẫn DB local
  - đường dẫn file gói đã tải
- Giải thích rõ việc update không làm mất dữ liệu vì SQLite ở `AppDataLocation`

## 9) ReportsScreen

- Dạng `Flickable`, chia theo 3 khối báo cáo lớn:
  - `Sales Summary Theo Kỳ`
  - `Debt Aging Report`
  - `Inventory Movement`
- Mỗi khối đều có action export `CSV` và `PDF`

### Sales Summary Theo Kỳ
- nhập `Từ ngày`, `Đến ngày`
- card số liệu:
  - số đơn
  - doanh số
  - đã thu
  - còn phải thu
- list chi tiết theo ngày

### Debt Aging Report
- card bucket:
  - `0-30 ngày`
  - `31-60 ngày`
  - `>60 ngày`
  - tổng công nợ
- list đối chiếu theo khách

### Inventory Movement
- lọc theo:
  - từ khóa SKU/tên sản phẩm/lô/lý do
  - từ ngày/đến ngày
- list lịch sử biến động tồn theo bộ lọc

### Trạng thái phụ
- banner thành công khi export xong, hiển thị `lastExportPath`
- banner lỗi dùng `lastError`
