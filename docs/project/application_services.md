# Application Services

## 1) DatabaseService (`infra/db`)

- API hiện có:
  - `initialize()`
  - `isReady()`
  - `backendName()`
- Trạng thái: placeholder cho SQLite local-first.

## 2) CustomerService

### Chức năng
- Tìm kiếm khách (`findCustomers`) theo mã/tên/sđt.
- Kiểm tra tồn tại (`customerExists`), lấy tên theo id.
- CRUD khách hàng.
- Sinh mã tự động dạng `KH0001`, `KH0002`, ...
- Seed dữ liệu mẫu khi khởi tạo service.

### Validation chính
- Tên khách bắt buộc.
- Hạn mức công nợ phải `>= 0`.

## 3) ProductService

### Chức năng
- Tìm kiếm sản phẩm (`findProducts`) theo SKU/tên/đơn vị.
- CRUD sản phẩm.
- Quản lý lô theo từng sản phẩm (`addLot`, `findLotsByProduct`).
- Truy xuất lịch sử biến động tồn theo sản phẩm (`findInventoryMovementsByProduct`).
- Nhập/xuất tồn theo lô (`stockIn`, `stockOut`).
- Điều chỉnh tồn theo lô có lý do (`adjustLotQuantity`).
- Tính tồn tổng theo sản phẩm (`totalOnHandByProduct`).
- Đếm sản phẩm sắp hết hàng (`lowStockProductCount`).
- Đếm mặt hàng có lô sắp hết hạn trong vòng 30 ngày (`expiringSoonProductCount`).

### Quy tắc chính
- SKU chuẩn hóa uppercase, bỏ khoảng trắng.
- SKU auto-gen khi tạo mới nếu để trống: `SP0001`, `SP0002`, ...
- Mã lô auto-gen khi để trống: `L00001`, `L00002`, ...
- Lô bắt buộc có `ngày hết hạn` và phải theo format `YYYY-MM-DD`.
- `ngày hết hạn` phải lớn hơn hoặc bằng `ngày nhập`.
- Không cho xóa sản phẩm nếu còn tồn kho.
- Không cho xuất lô khi không đủ tồn.
- Không cho điều chỉnh làm tồn âm.
- Điều chỉnh tồn bắt buộc phải có lý do.
- Mọi biến động tồn thành công đều ghi `InventoryMovement`:
  - `LotCreated`
  - `StockIn`
  - `StockOut`
  - `AdjustUp`
  - `AdjustDown`

## 4) OrderService

### Chức năng
- Tạo draft order (`createDraftOrder`) với mã `ORD00001`, `ORD00002`, ...
- Cập nhật khách của draft (`updateDraftOrderCustomer`).
- Upsert item draft theo `productId` (`upsertDraftItem`).
- Xóa item draft (`removeDraftItem`).
- Confirm order (`confirmOrder`) -> trừ kho theo lô.
- Void order (`voidOrder`) -> hoàn kho (nếu đơn đã confirm).
- Tải dashboard metrics (`loadDashboardMetrics`).
  - gồm cả cảnh báo mặt hàng có lô sắp hết hạn trong 30 ngày.

### Cách trừ kho khi confirm
- Lập kế hoạch cấp phát từ các lô của sản phẩm.
- Sắp theo `receivedDate` tăng dần, rồi `lotNo` tăng dần.
- Trừ kho lần lượt đến khi đủ qty item.
- Nếu trừ kho giữa chừng lỗi: rollback các lô đã trừ trước đó.
- Khi confirm thành công, mỗi lần trừ kho đều ghi movement với lý do dạng `Xác nhận đơn <orderNo>`.

### Cách hoàn kho khi void
- Chỉ cho void `Draft` hoặc `Confirmed`.
- Nếu trạng thái `Confirmed`: hoàn kho theo danh sách allocation đã lưu.
- Nếu hoàn kho lỗi giữa chừng: rollback các thao tác hoàn trước đó.
- Khi void đơn đã confirm, thao tác hoàn kho ghi movement với lý do dạng `Void đơn <orderNo>`.

### Validation chính
- Chỉ `Draft` mới được chỉnh item/khách.
- Không confirm draft rỗng (không có item).
- Không cho discount vượt line subtotal.
- Không cho qty <= 0, money âm.
- Chỉ đơn `Confirmed/PartiallyPaid` mới được nhận thanh toán.
- Không cho thu vượt `balanceVnd`.

## 5) PaymentService

### Chức năng
- Tính danh sách khách có công nợ hiện tại (`findCustomerReceivables`).
- Lấy danh sách đơn còn phải thu theo khách (`findPayableOrdersByCustomer`).
- Lấy toàn bộ phiếu thu đã lập (`findAllPayments`) để phục vụ dashboard/analytics.
- Lập phiếu thu (`createReceipt`).
- Lưu lịch sử phiếu thu in-memory.
- Dựng ledger đối soát theo khách (`findLedgerByCustomer`) từ:
  - bút toán phát sinh công nợ của đơn
  - bút toán giảm công nợ từ phiếu thu

### Quy tắc chính
- Chỉ lập phiếu thu cho đơn thuộc đúng khách đã chọn.
- Chỉ nhận thanh toán khi đơn đang ở `Confirmed` hoặc `PartiallyPaid`.
- Chặn over-payment trước khi ghi phiếu thu.
- `paidAt` phải đúng định dạng `YYYY-MM-DD` (hoặc để trống để lấy ngày hiện tại).
- Khi thu tiền thành công:
  - cập nhật `paidVnd`
  - cập nhật `balanceVnd`
  - tự đổi trạng thái đơn sang `PartiallyPaid` hoặc `Paid`

## 6) ReportService

### Chức năng
- Tạo `SalesSummaryReport` theo khoảng ngày:
  - tổng số đơn
  - tổng doanh số
  - tổng tiền thu
  - công nợ còn lại
  - chi tiết theo ngày
- Tạo `DebtAgingReport`:
  - chia bucket `0-30`, `31-60`, `>60 ngày`
  - đối chiếu theo từng khách còn công nợ
- Tạo `InventoryMovementReport`:
  - lọc theo từ ngày/đến ngày
  - lọc theo từ khóa SKU/tên sản phẩm/lô/lý do
- Export từng báo cáo ra:
  - `CSV`
  - `PDF`

### Quy tắc chính
- Nếu bỏ trống cả khoảng ngày:
  - mặc định lấy từ đầu tháng hiện tại đến hôm nay
- Nếu chỉ nhập 1 mốc ngày:
  - hệ thống hiểu là báo cáo cho đúng ngày đó
- Chỉ tính doanh số với đơn `Confirmed`, `PartiallyPaid`, `Paid`
- `Debt aging` chỉ tính các đơn còn `balanceVnd > 0`

## 7) Giới hạn hiện tại

- Service layer dùng in-memory vector/hash.
- Chưa có transaction DB thật sự.
- Phiếu thu và ledger hiện chưa persistence xuống SQLite.
- Inventory movement hiện cũng mới ở mức in-memory.
