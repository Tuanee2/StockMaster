# ViewModels

## 1) AppViewModel

Expose cho shell/UI chung:
- section navigation (`currentSection`, `sections`)
- DB trạng thái (`databaseReady`, `databaseBackend`)
- metrics dashboard (`customerCount`, `productCount`, `openOrderCount`, `lowStockCount`, `expiringSoonProductCount`, `receivableVndText`)
- dashboard analytics:
  - `operationalAlerts`
  - `monthlyTrends`
  - `monthlyTrendPeakVnd`
  - `topCustomers`
  - `topProducts`
  - `hasMonthlyActivity`

Nguồn dữ liệu:
- metrics snapshot: `OrderService::loadDashboardMetrics()`
- analytics/tổng hợp dashboard: gom từ `OrderService`, `PaymentService`, `CustomerService`, `ProductService`

## 2) CustomersViewModel

- Kiểu: `QAbstractListModel`
- Role chính: `customerId`, `code`, `name`, `phone`, `address`, `creditLimitText`
- Action:
  - `createCustomer`
  - `updateCustomer`
  - `deleteCustomer`
  - `reload`
  - `getCustomer(row)`
- Có `filterText` để lọc realtime.

Validation input tiền:
- bỏ khoảng trắng/dấu phân tách
- chỉ chấp nhận số nguyên `>= 0`

## 3) ProductsViewModel

- Kiểu: `QAbstractListModel`
- Role chính: `productId`, `sku`, `name`, `unit`, `defaultWholesalePriceText`, `totalOnHand`, `lotCount`
- Action sản phẩm:
  - `createProduct`, `updateProduct`, `deleteProduct`
  - `selectProduct`, `reload`, `getProduct(row)`
- Action lô/tồn:
  - `addLotToSelectedProduct`
  - `stockInLot`
  - `stockOutLot`
- Expose `selectedProductLots` dạng `QVariantList` để QML render list lô.
- Expose thêm `selectedProductExpiringSoonLotCount` để hiện cảnh báo theo sản phẩm đang chọn.
- Khi tạo mới, UI có thể để trống SKU; `ProductService` sẽ tự sinh mã.

Mỗi lot trong `selectedProductLots` có thêm dữ liệu:
- `expiryDate`
- `daysToExpiry`
- `isExpiringSoon` (<= 30 ngày, chưa quá hạn, còn tồn)
- `isExpired`

## 4) InventoryViewModel

- Kiểu: `QAbstractListModel`
- Role chính list snapshot:
  - `productId`, `sku`, `name`, `unit`, `totalOnHand`, `lotCount`, `isLowStock`

### Action inventory
- `reload`
- `selectProduct`
- `adjustSelectedLot`

### Data expose cho UI
- `productCount`, `lowStockCount`
- `selectedProductId`, `selectedProductName`
- `selectedProductTotalOnHand`, `selectedProductLowStock`
- `selectedProductLots`: snapshot theo lô của sản phẩm đang chọn
- `selectedProductMovements`: lịch sử biến động tồn của sản phẩm đang chọn

### Rule chính
- Lọc realtime theo SKU/tên/đơn vị qua `filterText`.
- `adjustSelectedLot` chỉ nhận số nguyên khác 0.
- Điều chỉnh âm vượt tồn sẽ bị chặn.
- Lỗi validate/nghiệp vụ trả qua `lastError`.

## 5) PaymentsViewModel

- Kiểu: `QAbstractListModel`
- Role chính list khách:
  - `customerId`, `code`, `name`, `receivableText`, `openOrderCount`, `paymentCount`

### Action payment
- `selectCustomer`
- `createReceipt`
- `reload`

### Data expose cho UI
- `payableOrders`: các đơn `Confirmed/PartiallyPaid` còn công nợ
- `paymentHistory`: lịch sử phiếu thu của khách đang chọn
- `customerLedger`: ledger đối soát theo khách đang chọn

### Rule chính
- Chỉ cho lập phiếu thu khi đã chọn khách.
- Số tiền thu phải là số nguyên `> 0`.
- Lỗi nghiệp vụ (over-payment, sai ngày, sai trạng thái đơn) đẩy ra `lastError`.

## 6) OrdersViewModel

- Kiểu: `QAbstractListModel`
- Role chính list đơn:
  - `orderId`, `orderNo`, `orderDate`, `status`, `customerName`, `totalText`, `itemCount`

### Action order
- `createDraftOrder`
- `selectOrder`
- `updateSelectedOrderCustomer`
- `addOrUpdateDraftItem`
- `removeDraftItem`
- `confirmSelectedOrder`
- `voidSelectedOrder`
- `lotsForProduct`
- `previewAllocations`

### Query danh sách đơn
- Property query:
  - `queryOrderNo`
  - `queryFromDate`
  - `queryToDate`
- API:
  - `applyOrderQuery(orderNo, fromDate, toDate)`
  - `clearOrderQuery()`

### Rule lọc
- Mã đơn: contains, không phân biệt hoa/thường.
- Ngày:
  - chỉ nhập `fromDate` -> lọc đúng ngày đó
  - chỉ nhập `toDate` -> lọc đúng ngày đó
  - nhập cả 2 -> lọc theo khoảng `from <= orderDate <= to`
- Format ngày bắt buộc `YYYY-MM-DD`, validate và báo lỗi qua `lastError`.

### Trạng thái chọn đơn
- Expose `canEditDraft`, `canConfirm`, `canVoid`, `canOpenPayment` để bật/tắt action trên UI.

### Rule item draft
- `addOrUpdateDraftItem` nhận thêm `preferredLotId`.
- UI có thể xem trước cách hệ thống phân bổ qty từ lô đã chọn sang các lô còn lại.
- `selectedOrderItems` có thêm `lotSummary` và `primaryLotId` để hiển thị/đổ ngược form.

## 7) ReportsViewModel

- Kiểu: `QObject`
- Expose 3 nhóm dữ liệu báo cáo:
  - `sales summary`
  - `debt aging`
  - `inventory movement`
- Expose thêm:
  - `lastError`
  - `lastExportPath`

### Action report
- `reloadAll`
- `loadSalesSummary(fromDate, toDate)`
- `loadDebtAging()`
- `loadInventoryMovement(keyword, fromDate, toDate)`
- `exportSalesSummary(format)`
- `exportDebtAging(format)`
- `exportInventoryMovement(format)`

### Quy ước export
- `format = "csv"` hoặc `format = "pdf"`
- file export mặc định được tạo trong thư mục:
  - `Documents/StockMasterExports`
  - fallback sang thư mục chạy app nếu không lấy được `Documents`

## 8) SettingsViewModel

- Kiểu: `QObject`
- Mục tiêu: làm updater tối giản cho bản desktop hiện tại

### Data expose cho UI
- `currentVersion`
- `latestVersion`
- `statusText`
- `updateActionLabel`
- `databasePath`
- `downloadedFilePath`
- `checking`, `downloading`, `busy`
- `updateAvailable`
- `canUpdate`
- `downloadProgress`

### Action
- `checkForUpdates()`
- `downloadUpdate()`

### Rule chính
- Gọi GitHub Releases API để đọc bản `latest`
- So sánh `tag_name` với `QCoreApplication::applicationVersion()`
- `currentVersion` và `latestVersion` được format về semantic version `x.y.z` trước khi hiển thị trên UI
- Nếu có bản mới và có asset đúng nền tảng:
  - bật trạng thái sẵn sàng cập nhật cho UI
  - chỉ khi người dùng bấm `Cập nhật` mới tải gói về `Downloads/StockMasterUpdates`
  - sau khi tải xong sẽ thử mở gói cập nhật
- Không đụng file SQLite hiện có vì DB local nằm ở `AppDataLocation`
