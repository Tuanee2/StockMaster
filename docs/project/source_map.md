# Source Map

## 1) Entry points

- `main.cpp`: khởi tạo app + service + viewmodels + inject context vào QML.
- `CMakeLists.txt`: định nghĩa executable, sources, QML module.

## 2) Cấu trúc thư mục chính

```text
tests/
src/stockmaster/
  core/
  domain/
  application/
  infra/db/
  ui/
    viewmodels/
    qml/
      components/
      screens/
```

## 3) Trách nhiệm từng vùng

- `core/`
  - kiểu cơ bản dùng toàn hệ thống (`Money`).

- `domain/`
  - các struct nghiệp vụ: `Customer`, `Product`, `ProductLot`, `InventoryMovement`, `Order`, `OrderItem`, ...

- `application/`
  - `CustomerService`: CRUD khách hàng.
  - `ProductService`: CRUD sản phẩm + lô + nhập/xuất tồn + lịch sử biến động + điều chỉnh tồn.
  - `OrderService`: lifecycle đơn và thao tác trừ/hoàn kho khi confirm/void.
  - `PaymentService`: lập phiếu thu, chống over-payment, dựng ledger đối soát theo khách, cung cấp payment history cho dashboard.
  - `ReportService`: tổng hợp sales/debt/movement report và export CSV/PDF.

- `infra/db/`
  - `DatabaseService`: mở kết nối `QSQLITE`, bootstrap schema local, quản lý transaction lồng nhau mức đơn giản.

- `ui/viewmodels/`
  - map nghiệp vụ sang API dễ bind cho QML (`AppViewModel`, `CustomersViewModel`, `ProductsViewModel`, `InventoryViewModel`, `OrdersViewModel`, `PaymentsViewModel`, `ReportsViewModel`).

- `ui/qml/`
  - shell/navigation + screen + reusable component.

- `tests/`
  - smoke test mức service cho các rule nghiệp vụ cốt lõi.

## 4) Wiring runtime (tóm tắt)

`main.cpp` khởi tạo theo thứ tự:
1. `DatabaseService`
2. `CustomerService` (load/seed khách từ SQLite)
3. `ProductService` (load/seed sản phẩm, lô, movement từ SQLite)
4. `OrderService` (load đơn, dòng đơn, allocation từ SQLite)
5. `PaymentService` (load phiếu thu từ SQLite)
6. `ReportService`
7. `AppViewModel`, `CustomersViewModel`, `InventoryViewModel`, `OrdersViewModel`, `PaymentsViewModel`, `ProductsViewModel`, `ReportsViewModel`
8. expose vào QML context (`appViewModel`, `customersViewModel`, `inventoryViewModel`, `ordersViewModel`, `paymentsViewModel`, `productsViewModel`, `reportsViewModel`)

`AppViewModel` hiện tổng hợp dữ liệu dashboard từ:
- `OrderService`
- `PaymentService`
- `CustomerService`
- `ProductService`
