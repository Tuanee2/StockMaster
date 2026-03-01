# Project Docs Index — StockMaster

Tài liệu trong thư mục này mô tả trạng thái triển khai **thực tế theo code hiện tại**.

## Reading Order (Trong `docs/project`)

1. `docs/project/overview.md`
2. `docs/project/build_and_run.md`
3. `docs/project/user_guide.md`
4. `docs/project/source_map.md`
5. `docs/project/domain_and_core.md`
6. `docs/project/application_services.md`
7. `docs/project/viewmodels.md`
8. `docs/project/ui_screens.md`
9. `docs/project/current_features_and_gaps.md`

## Mục tiêu bộ tài liệu

- Giúp onboard nhanh mà không cần đọc toàn bộ source ngay từ đầu.
- Có sẵn tài liệu thao tác cho người dùng cuối.
- Ghi rõ phần nào đã chạy thực tế, phần nào còn placeholder.
- Làm checklist đối chiếu trước khi mở rộng feature.

## Quy ước cập nhật

- Sau mỗi lần đổi code và build pass, cập nhật ít nhất các file liên quan trực tiếp.
- Nếu đổi luồng nghiệp vụ lớn (Order/Product/Inventory/Payment/Report): cập nhật cả
  `application_services.md`, `viewmodels.md`, `ui_screens.md`.
