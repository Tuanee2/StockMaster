# Folder Structure — StockMaster

Mục tiêu: agent luôn đặt code đúng chỗ, không tạo “thư mục rác”, giữ kiến trúc ổn định.

---

## Target layout (khuyến nghị)

```text
src/
└── stockmaster/
    ├── core/          # utils, Result, logging, time, id generator
    ├── domain/        # entities, value objects, invariants
    ├── application/   # use cases, application services
    ├── infra/
    │   ├── db/        # sqlite impl, migrations, queries
    │   ├── sync/      # supabase impl, conflict policy, sync pipeline
    │   └── report/    # export/import/pdf/csv
    └── ui/
        ├── viewmodels/  # QObject viewmodels exposed to QML
        └── qml/
            ├── components/
            ├── screens/
            └── assets/
```

---

## What goes where

### core/
- Result/Expected
- Error types
- Utilities (time/id, formatting NON-UI)

### domain/
- Customer/Product/Order/Payment value rules
- State machine constraints

### application/
- UC-xxx flows trong `docs/use_cases.md`
- Transaction boundaries
- Calls repo interfaces

### infra/db/
- SQLite schema, migrations, queries
- Repository implementations

### infra/sync/
- Supabase client
- Pull/push pipeline
- Conflict policy

### ui/viewmodels/
- Q_PROPERTY state
- Actions gọi usecases

### ui/qml/
- screens: mỗi màn hình 1 file
- components: reusable controls (MacButton/MacCard…)
- assets: icons/images

---

## Naming conventions

- Classes: PascalCase (OrderService, InventoryService, OrdersViewModel)
- Files: snake_case hoặc PascalCase (chọn 1 và giữ nhất quán)
- QML components: PascalCase (MacButton.qml)

---

## Forbidden

- Không tạo `src/ui/` hoặc `src/viewmodels/` song song nếu đã có `src/stockmaster/ui/`
- Không đặt SQL trong ui/ hoặc viewmodels/
- Không viết business rules trong QML