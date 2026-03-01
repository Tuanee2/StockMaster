# Decisions — StockMaster (ADR-lite)

File này chứa các quyết định kiến trúc để agent KHÔNG tự suy diễn.

---

## D-0001 Currency Type

Date: 2026-02-22  
Status: Accepted

Decision:
- Currency mặc định: VND
- Money lưu bằng int64 (đơn vị đồng)
- Không dùng double/float

---

## D-0002 Local-first

Date: 2026-02-22  
Status: Accepted

Decision:
- SQLite là source of truth
- Sync Supabase là optional

---

## D-0003 DB Thread Model

Status: Proposed

Decision:
- 1 DbWorker thread xử lý database

---

## D-0004 Stock Policy

Status: Proposed

Options:

A — Confirm trừ kho  
B — Deliver trừ kho

Default hiện tại: A (MVP đơn giản)

---

## D-0005 Debt Policy

Status: Proposed

Options:

A — Ledger required  
B — Compute từ Order + Payment

Default: A

---

## D-0006 Soft Delete

Status: Proposed

Decision:
- Sử dụng deleted_at
- UI mặc định ẩn record đã xóa

---

## Template