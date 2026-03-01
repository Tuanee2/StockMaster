# Docs Index — StockMaster

File này là bản đồ tài liệu chính thức cho agent và developer.

## Reading Order (Bắt buộc)

1. `AGENTS.md`
2. `docs/_index.md`
3. `docs/agent_rules.md`
4. `docs/folder_structure.md`
5. `docs/ui_guidelines.md`
6. `docs/decisions.md`
7. `docs/architecture.md`
8. `docs/data_model.md`
9. `docs/use_case.md`
10. `docs/project/_index.md`
11. `src/` (code là source of truth)

Nguyên tắc:
- Khi docs và code mâu thuẫn: tin code.
- Khi đổi feature hoặc UI flow: cập nhật `docs/project/*` sau khi build thành công.

## Task -> File Mapping

### Thêm hoặc sửa nghiệp vụ Order/Product/Customer
Đọc:
- `docs/use_case.md`
- `docs/data_model.md`
- `docs/project/application_services.md`
- `docs/project/viewmodels.md`

### Sửa UI màn hình
Đọc:
- `docs/ui_guidelines.md`
- `docs/project/ui_screens.md`

### Sửa kiến trúc hoặc wiring app
Đọc:
- `docs/architecture.md`
- `docs/project/source_map.md`
- `docs/project/overview.md`

### Sửa build/run
Đọc:
- `docs/project/build_and_run.md`

### Cập nhật tổng quan project cho người mới
Đọc:
- `docs/project/_index.md`
- `docs/project/overview.md`
- `docs/project/current_features_and_gaps.md`

## Golden Rules

- Không dùng `double` cho tiền; dùng `Money` (`qint64`).
- Không nhét business logic vào QML.
- Các thao tác Order/Stock cần giữ tính nhất quán nghiệp vụ.
- Mọi thay đổi xong phải build pass.
