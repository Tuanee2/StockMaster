# UI Screens

## 1) Main và App Shell

- `Main.qml`: `ApplicationWindow` + mount `AppShell`.
- `Main.qml` mở ở trạng thái `maximized` để app phủ kín màn hình ngay khi khởi động.
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
  - biểu đồ cột đôi `Doanh số và thu tiền 7 ngày gần nhất`
    - cột được dựng từ đáy lên
    - rê chuột vào từng cột sẽ hiện tooltip giá trị thực tế
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
- Input và nút thao tác chính đã dùng cùng style `SolidTextField` / `ActionButton`
  như `Tồn kho` và `Đơn hàng` để giữ màu nền, border và chiều cao đồng bộ
- Sau khi thêm/sửa/xóa khách:
  - màn `Đơn hàng` và `Thanh toán` sẽ reload lookup để thấy dữ liệu khách mới ngay

## 4) ProductsScreen

- Bên trái:
  - tìm kiếm sản phẩm
  - list sản phẩm + tồn tổng + số lô
- Bên phải:
  - form CRUD sản phẩm
    - mã sản phẩm chỉ hiển thị, không nhập tay khi tạo mới (tự sinh khi lưu)
  - khối quản lý lô cho sản phẩm đang chọn
    - có `Ngày nhập` và `Hạn dùng` khi thêm lô
    - cảnh báo số lô sắp hết hạn trong 30 ngày
    - badge trạng thái `Quá hạn` hoặc `Còn N ngày` trên từng lô
  - khối nhập/xuất theo lô đang chọn
  - vùng báo lỗi `lastError`

UI đã được chỉnh:
- action button màu solid (không trong suốt)
- field sử dụng layout fill/min width để hạn chế tràn khi thu nhỏ
- form sản phẩm, thêm lô và nhập/xuất đang dùng cùng style field với `Tồn kho` / `Đơn hàng`

## 5) OrdersScreen

Màn hình chia 2 cột rõ ràng:

### Cột trái: Danh sách đơn
- Header "Danh sách đơn" + tổng đơn.
- Khối 1: "Tùy chọn lọc danh sách đơn"
  - lọc theo mã đơn
  - lọc theo từ ngày/đến ngày
  - nút `Lọc`, `Xóa lọc`
  - layout cố định 3 hàng:
    - hàng 1: mã đơn
    - hàng 2: từ ngày + đến ngày
    - hàng 3: lọc + xóa lọc
- Khối 2: "Danh sách đơn theo bộ lọc"
  - list đơn kết quả sau lọc

### Cột phải: Điều hướng nghiệp vụ Order
- Thanh tab nhỏ:
  - `Tạo Draft`
  - `Item & Xử lý`
  - `Chi tiết đơn`
- Tab `Tạo Draft`:
  - có ô tìm khách theo tên / mã / số điện thoại
  - tạo draft mới
- Tab `Item & Xử lý`:
  - thêm/cập nhật/xóa item
  - chọn sản phẩm + chọn lô ưu tiên
  - xem preview phân bổ qty qua nhiều lô nếu lô đang chọn không đủ
  - confirm (trừ kho)
  - void (hoàn kho)
  - có nút nhảy thẳng sang `Thanh toán` cho đơn đang chọn
- Tab `Chi tiết đơn`:
  - có ô tìm khách theo tên / số điện thoại
  - xem và đổi khách hàng của draft
- Có `lastError` banner hiển thị lỗi nghiệp vụ.

UI đã được chỉnh:
- textfield/combo custom solid background
- màu chữ và placeholder tương phản nền sáng
- layout ngang, không dùng bố cục trên/dưới như bản cũ
- khi chuyển vào màn `Đơn hàng`, viewmodel sẽ reload lại lookup khách/sản phẩm để tránh giữ cache cũ

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
    - layout theo 3 hàng rõ ràng:
      - hàng 1: chọn đơn còn công nợ
      - hàng 2: số tiền thu + phương thức
      - hàng 3: ngày thu + nút lập phiếu thu
  - khối lịch sử phiếu thu
  - khối ledger đối soát
  - vùng báo lỗi `lastError`

Layout note:
- card `Công nợ hiện tại` và `Lập phiếu thu` tự tính chiều cao theo nội dung để không bị khối lịch sử/ledger đè lên khi layout co giãn.

Rule UI đang phản ánh:
- không cho lập phiếu thu nếu khách không có đơn chờ thu
- nhắc rõ phiếu thu tự chặn over-payment
- sau khi thu tiền, dashboard và công nợ cập nhật lại
- khi chuyển vào màn `Thanh toán`, danh sách khách sẽ reload lại để nhận dữ liệu mới nhất
- nếu được điều hướng từ `Đơn hàng`, combobox đơn sẽ ưu tiên chọn đúng đơn vừa chuyển sang

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
    - layout theo 3 hàng rõ ràng:
      - hàng 1: chọn lô
      - hàng 2: số lượng + lý do
      - hàng 3: ngày điều chỉnh + nút áp dụng
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
- Không tự kiểm tra khi vừa mở tab.
- Trong phần nội dung có 2 nút thao tác:
  - `Kiểm tra phiên bản`
  - `Cập nhật`
- Nút `Cập nhật` chỉ bật khi:
  - đã kiểm tra xong
  - có bản mới
  - có gói đúng nền tảng hiện tại
- Khi bấm `Cập nhật`:
  - app mới bắt đầu tải gói cập nhật
  - sau khi tải xong sẽ tự mở gói vừa tải
- Hiển thị:
  - version hiện tại (format semantic `x.y.z`)
  - version mới nhất trên release (format semantic `x.y.z`)
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
