

# StockMaster Architecture

Tài liệu này mô tả kiến trúc đề xuất cho **StockMaster** — ứng dụng quản lý bán buôn (desktop + mobile) theo hướng **local-first** với **SQLite** và tùy chọn **sync Supabase**.

## Goals

- **Correctness trước**: tiền tệ, công nợ, tồn kho, trạng thái đơn hàng phải nhất quán.
- **UI mượt**: không block UI thread khi query DB, import/export, sync.
- **Scale được**: dữ liệu lớn (hàng nghìn – hàng trăm nghìn bản ghi), nhiều thiết bị, nhiều user.
- **Dễ test**: rule nghiệp vụ nằm ở Domain/UseCase, tách khỏi QML.

## Non-goals (để tránh phình scope)

- Không làm ERP đầy đủ ngay từ đầu (mua hàng, sản xuất, kế toán chuẩn…)
- Không xây sync phức tạp kiểu CRDT ngay lập tức.

## High-level Architecture

Áp dụng mô hình **Layered + MVVM-lite**:

```
QML UI (Views)  <->  ViewModels (QObjects, Q_PROPERTY)
                         |
                         v
                 UseCases / Application Services
                         |
                         v
         Repositories (interfaces)  ->  Infrastructure
                         |
                         v
              SQLite (local)  +  Supabase (optional)
```

### Layer responsibilities

1. **UI (QML)**
   - Chỉ lo hiển thị, điều hướng, binding.
   - Không query DB trực tiếp.
   - Không chứa rule nghiệp vụ.

2. **ViewModel / Controller (Qt QObject)**
   - Expose state cho UI qua `Q_PROPERTY` và `NOTIFY`.
   - Nhận input từ UI, validate nhẹ (format, required, range).
   - Gọi UseCase để thực thi.
   - Không chứa SQL.

3. **UseCases / Application Services**
   - Luồng nghiệp vụ: tạo đơn, xác nhận đơn, thanh toán, xuất kho…
   - Gộp nhiều thao tác repo trong **transaction**.
   - Là nơi tốt nhất để viết unit test.

4. **Domain**
   - Entity/ValueObject + rule cốt lõi.
   - Tiền tệ: dùng integer minor unit (VND) hoặc decimal an toàn.

5. **Repositories (interfaces)**
   - Hợp đồng truy xuất dữ liệu: `OrderRepository`, `ProductRepository`…
   - Cho phép thay implementation (SQLite/Supabase/mock).

6. **Infrastructure**
   - SQLite implementation (SQL, migrations, queries).
   - Supabase client (REST/PostgREST/Realtime tùy chọn).
   - Export/Import, Report generation.

## Module layout (đề xuất)

```
src/
  stockmaster/
    core/          # utilities, logging, Result<T>, time, id generator
    domain/        # entities, value objects, rules
    application/   # usecases, services (OrderService, InventoryService...)
    infra/
      db/          # sqlite impl, migrations
      sync/        # supabase impl, conflict policy
      report/      # pdf/csv exporters
    ui/
      viewmodels/  # QObjects exposed to QML
      qml/         # screens, components
```

> Nếu repo hiện chưa theo cấu trúc này: ưu tiên chuyển dần theo feature (Orders, Products…) rồi chuẩn hóa sau.

## Data model (core entities)

Tối thiểu cho bản MVP:

- **Customer**: thông tin khách, giới hạn công nợ (optional)
- **Product**: SKU, tên, đơn vị, giá sỉ mặc định
- **Inventory**: tồn kho theo sản phẩm (và theo kho nếu multi-warehouse)
- **Order**: header (khách, ngày, trạng thái, tổng tiền)
- **OrderItem**: line items (product, qty, unit price, discount)
- **Payment**: phiếu thu/chi gắn với order hoặc customer
- **DebtLedger**: sổ cái công nợ (khuyến nghị để đối soát)

### Money & rounding

- VND: lưu tiền bằng `int64` (đơn vị đồng).
- Nếu cần nhiều currency sau này: lưu (amount_minor, currency_code).
- Không dùng `double` cho tiền.

## Order & Inventory workflow

Đề xuất trạng thái đơn hàng (có thể đổi theo nhu cầu):

- `Draft` → `Confirmed` → `Paid` (hoặc `PartiallyPaid`) → `Completed`
- `Voided/Cancelled` (có lý do)

### Nguyên tắc tồn kho

- Khi `Confirmed`: trừ tồn kho (hoặc tạo phiếu xuất kho), tùy policy.
- Khi `Voided`: hoàn tồn kho.
- Mọi cập nhật tồn kho phải ở trong transaction.

## Threading model

**Rule**: UI thread không được chạy SQL nặng.

### Option A (khuyến nghị): Single DB worker thread

- Tạo `DbWorker` chạy `QThread` riêng.
- Mọi request DB đi qua queue (signal/slot queued connection).
- Trả kết quả về UI bằng signal.

Ưu điểm: đơn giản, ít lỗi thread-safety với SQLite.

### Option B: Per-thread connection

- Mỗi thread có `QSqlDatabase` connection riêng.
- Cần discipline cao để tránh sử dụng chéo connection.

## Database layer (SQLite)

- **Migrations**: dùng version table + scripts.
- Queries lớn: paginate (LIMIT/OFFSET) hoặc keyset pagination.
- Index: customer_id, product_id, order_date, status…

### Transaction guideline

UseCase phải bao transaction cho các thao tác liên quan:

- Create Order + OrderItems + update Inventory + append DebtLedger
- Apply Payment + update Order balance + append DebtLedger

## Sync (Supabase) — optional

Nếu bật sync, ưu tiên chiến lược **đơn giản nhưng an toàn**:

- Mỗi record có: `id`, `created_at`, `updated_at`, `deleted_at` (soft delete), `device_id`.
- Conflict policy (default): **Last-Write-Wins** theo `updated_at` (server time tốt hơn).
- Các bảng quan trọng (Order/Payment/DebtLedger) nên có quy tắc tránh merge sai:
  - Hạn chế sửa trực tiếp record đã "final" (Paid/Completed) → thay bằng record điều chỉnh (adjustment).

### Sync pipeline

1. Pull remote changes (since last sync)
2. Apply to local (transaction)
3. Push local pending changes
4. Update sync cursor

> Luôn log sync theo mức debug (không chứa token/PII).

## Navigation & Screens

Tối thiểu:

- Dashboard
- Customers (list/detail)
- Products (list/detail)
- Orders (list/create/detail)
- Payments/Debt
- Inventory
- Reports
- Settings (DB backup/restore, sync)

QML đề xuất pattern:

- `AppShell.qml` chứa sidebar/topbar
- Mỗi screen là 1 file `screens/*.qml`
- Component dùng lại: `components/*`

## Validation strategy

- UI: required fields, format input.
- Application: validate business rules (credit limit, stock availability, order state transitions).
- Domain: invariants (Money, Quantity non-negative…)

## Error handling

Chuẩn hóa `Result<T>` hoặc `Expected<T, Error>` để:

- UseCase trả lỗi rõ ràng (code + message)
- VM map lỗi → UI (toast/dialog)

## Logging

- Core: log category theo module (`db`, `sync`, `order`, `ui`…)
- Không log token, không log raw customer PII.

## Testing

- Domain unit tests: Money, Quantity, state transitions.
- Application tests: create order, void order, apply payment.
- Infra tests (smoke): migration + basic CRUD.

## MVP roadmap (gợi ý)

1. SQLite schema + migrations
2. Product + Customer CRUD
3. Order create/confirm + Inventory update
4. Payment + Debt ledger
5. Reports (CSV/PDF)
6. Optional: Supabase sync

---

## Decisions log

Ghi các quyết định quan trọng tại đây để team/agent không hiểu sai:

- Currency default: VND, store as `int64`.
- DB threading: (chọn A/B) …
- Stock policy: (confirm trừ kho / delivery trừ kho) …
- Debt policy: (ledger required / computed) …

## UI Design System — macOS Style

StockMaster sử dụng phong cách thiết kế lấy cảm hứng từ macOS:

### Principles

- Minimal
- Clean spacing
- Subtle shadows
- Soft rounded corners
- Focus vào content, không dùng màu quá gắt

---

### Layout Rules

- Sidebar trái giống macOS Finder
- Toolbar top dạng phẳng (flat)
- Content nằm trong card bo góc nhẹ
- Padding lớn hơn mặc định Qt

---

### Visual Rules

- Radius: 8px — 12px
- Shadow: nhẹ, blur lớn
- Border: 1px rất mờ (#FFFFFF10 hoặc #00000010)
- Không dùng viền dày

---

### Interaction Rules

- Hover: đổi màu nền rất nhẹ
- Click animation: scale nhỏ hoặc opacity
- Scroll mượt, tránh jump layout

---

### QML Implementation Guidelines

Agent khi viết UI phải:

- Dùng Rectangle + layer.effect thay vì border dày
- Tránh dùng Button style mặc định Qt
- Tạo custom component:

components/
    MacButton.qml
    MacCard.qml
    MacSidebar.qml
    MacToolbar.qml

---

### Colors (Default)

Background: #1C1C1E
Card: #2C2C2E
Accent: system accent
Text primary: #FFFFFF
Text secondary: #AAAAAA

---

### Forbidden

- Không dùng Material style
- Không dùng Fusion style
- Không dùng gradient mạnh
- Không dùng icon quá nhiều màu