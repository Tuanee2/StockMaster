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

- Project đang dùng Qt6 Quick module.
- QML được đóng gói qua `qt_add_qml_module` trong `CMakeLists.txt`.
- Sau khi thay đổi UI hoặc ViewModel, nên build lại ngay để bắt lỗi QML/C++ binding.

## 4) Chạy smoke test

Sau khi build xong, có thể chạy smoke test:

```bash
ctest --test-dir /Users/tuan/Documents/developer/StockMaster/StockMaster/build/Qt_6_9_3_for_macOS-Debug --output-on-failure
```

Test hiện có:
- `report_service_smoke`: kiểm tra sales/debt/movement report và export CSV/PDF tạo file thành công.
- `dashboard_view_model_smoke`: kiểm tra dashboard analytics có dữ liệu trend/top list sau khi tạo đơn và phiếu thu.
- `inventory_service_smoke`: kiểm tra không cho điều chỉnh âm tồn, và ghi lịch sử biến động khi điều chỉnh/xuất.
- `payment_service_smoke`: kiểm tra partial payment, chặn over-payment, và auto chuyển trạng thái sang `Paid`.
