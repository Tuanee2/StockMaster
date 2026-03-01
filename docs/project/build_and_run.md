# Build And Run

## 1) Command build đang dùng và đã xác nhận

```bash
cmake --build /Users/tuan/Documents/developer/StockMaster/StockMaster/build/Qt_6_9_3_for_macOS-Debug --target all
```

Đây là lệnh build chính hiện tại của project trong môi trường dev.

## 2) Chạy app (sau khi build)

Binary macOS nằm tại:

```text
build/Qt_6_9_3_for_macOS-Debug/appStockMaster.app/Contents/MacOS/appStockMaster
```

## 3) Notes

- Project đang dùng Qt6 Quick + Qt6 Sql.
- App target hiện link thêm `Qt6 Network` để kiểm tra GitHub Release và tải gói cập nhật từ màn `Settings`.
- QML được đóng gói qua `qt_add_qml_module` trong `CMakeLists.txt`.
- Sau khi thay đổi UI hoặc ViewModel, nên build lại ngay để bắt lỗi QML/C++ binding.

## 4) SQLite local DB

- DB mặc định được lưu ở `QStandardPaths::AppDataLocation/stockmaster.sqlite`.
- Có thể override path file DB bằng biến môi trường:

```bash
export STOCKMASTER_DB_PATH=/duong-dan/stockmaster.sqlite
```

- Trong test smoke, mỗi test dùng file SQLite riêng trong thư mục tạm để tránh nhiễu dữ liệu.
- File DB nằm ngoài thư mục cài đặt, nên khi update app bằng gói mới thì dữ liệu cũ vẫn giữ nguyên.

## 5) Chạy smoke test

Sau khi build xong, có thể chạy smoke test:

```bash
ctest --test-dir /Users/tuan/Documents/developer/StockMaster/StockMaster/build/Qt_6_9_3_for_macOS-Debug --output-on-failure
```

Test hiện có:
- `report_service_smoke`: kiểm tra sales/debt/movement report và export CSV/PDF tạo file thành công.
- `sqlite_persistence_smoke`: kiểm tra dữ liệu đã ghi xuống SQLite vẫn load lại được ở phiên service mới.
- `dashboard_view_model_smoke`: kiểm tra dashboard analytics có dữ liệu trend/top list sau khi tạo đơn và phiếu thu.
- `inventory_service_smoke`: kiểm tra không cho điều chỉnh âm tồn, và ghi lịch sử biến động khi điều chỉnh/xuất.
- `payment_service_smoke`: kiểm tra partial payment, chặn over-payment, và auto chuyển trạng thái sang `Paid`.

## 6) Release workflow trên GitHub Actions

- Workflow: `.github/workflows/release.yml`
- Trigger:
  - `push` tag dạng `v*` để build và publish GitHub Release
  - `workflow_dispatch` để build/đóng gói thủ công
- Gói phát hành hiện tại:
  - macOS: `StockMaster-macos.dmg`
  - Windows: `StockMaster-windows-x64.zip`
- Pipeline:
  1. cài Qt `6.9.3`
  2. build `Release`
  3. macOS dùng `macdeployqt` để tạo `.dmg`
  4. Windows dùng `windeployqt` để gom DLL/plugin rồi nén `.zip`
  5. khi là tag `v*`, upload 2 file này vào GitHub Release

## 7) Updater trong Settings

- Khi mở tab `Settings`, app gọi GitHub Releases API:
  - `https://api.github.com/repos/Tuanee2/StockMaster/releases/latest`
- Nếu `tag_name` mới hơn version đang chạy và có asset phù hợp nền tảng:
  - macOS: ưu tiên `.dmg` / `.pkg`
  - Windows: ưu tiên `.zip` / `.msi` / `.exe`
- Gói cập nhật sẽ được tải về:
  - `Downloads/StockMasterUpdates`
  - fallback: `AppDataLocation/StockMasterUpdates`
- Sau khi tải xong, app sẽ thử mở gói vừa tải ngay bằng shell mặc định của hệ điều hành.
