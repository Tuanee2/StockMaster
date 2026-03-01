# Agent Rules — StockMaster (Qt/QML + SQLite + optional Sync)

File này là “luật chơi” để AI agent code đúng kiến trúc, không phá MVVM-lite, không nhét logic sai chỗ.

---

## 1) Source of truth

- **Code** là source of truth khi docs mâu thuẫn.
- Nhưng agent phải **update docs** nếu thay đổi kiến trúc/policy.

---

## 2) Layering rules (BẮT BUỘC)

### UI (QML)
✅ Được làm:
- Layout, styling, animation, navigation
- Binding properties từ ViewModel
- Gọi action của ViewModel (slot/invokable)

❌ Không được làm:
- Không viết SQL / không gọi DB trực tiếp
- Không gọi network/sync
- Không chứa business rules (tính tiền, công nợ, tồn kho, state machine)
- Không tự tạo “logic service” trong QML (JS file phức tạp)

---

### ViewModel (QObject + Q_PROPERTY)
✅ Được làm:
- Expose state cho UI
- Validate nhẹ: required/format/range
- Map Result/Error → UI state (loading/error/toast)
- Gọi UseCase

❌ Không được làm:
- Không viết SQL
- Không chạm trực tiếp QSqlDatabase
- Không chứa rule nghiệp vụ lõi (để vào UseCase/Domain)

---

### UseCases / Application services
✅ Được làm:
- Chứa flow nghiệp vụ (Create/Confirm/Void order, Apply payment…)
- Mở/đóng transaction
- Gọi repositories
- Trả về Result<T>

❌ Không được làm:
- Không biết UI/QML
- Không emit signal UI

---

### Domain
✅ Được làm:
- Entity, ValueObject, invariants
- Quy tắc tiền tệ, trạng thái hợp lệ, quantity constraints

❌ Không được làm:
- Không phụ thuộc Qt UI
- Hạn chế phụ thuộc DB/network

---

### Infrastructure (DB/Sync/Report)
✅ Được làm:
- SQL, migrations, queries
- Supabase client, sync pipeline
- Export/import/report

---

## 3) Threading rules

- UI thread **không được** chạy SQL nặng hoặc sync.
- DB access theo policy ở `docs/decisions.md` (khuyến nghị 1 DbWorker thread).
- Mọi callback về UI phải qua queued connection hoặc signal an toàn.

---

## 4) Money & numeric rules

- Money: **int64 VND** (không double/float).
- Tổng tiền/chiết khấu phải tính nhất quán, không làm tròn kiểu ngẫu nhiên.
- Không dùng `double` để lưu “giá” trong DB.

---

## 5) Transaction rules

Mọi thao tác liên quan các nhóm sau phải trong **1 transaction**:
- Order + OrderItems
- Inventory update (trừ/hoàn kho)
- Payment + Order balance
- DebtLedger append

---

## 6) Naming & structure rules

- Không tạo file lung tung. Tuân theo `docs/folder_structure.md`.
- Mỗi thay đổi phải đặt đúng layer.
- Không duplicate logic giữa nhiều ViewModel/UseCase.

---

## 7) UI consistency rules

- UI phải tuân theo `docs/ui_guidelines.md`.
- Mọi component dùng lại phải nằm trong `ui/qml/components/`.
- Không “style inline” bừa bãi trong mỗi screen.

---

## 8) Error handling

- UseCase trả `Result<T>` / `Expected<T, Error>` (một chuẩn duy nhất).
- ViewModel map error → UI state:
  - `loading` / `errorMessage` / `toast`
- Không throw exception xuyên layer (trừ khi repo đã chọn pattern đó và nhất quán).

---

## 9) Testing rules

- Thêm rule nghiệp vụ → phải có unit test (Domain/Application).
- Thay DB schema → smoke test migrate + basic CRUD.
- Không merge feature quan trọng mà không test tối thiểu.

---

## 10) Definition of done (code)

- Build pass
- No UI thread blocking
- Logic đúng theo UseCases + Decisions
- Docs cập nhật nếu thay đổi policy/flow