# Use Cases — StockMaster

Money = int64 VND  
Tất cả thay đổi tồn kho / công nợ / order phải dùng transaction.

---

## UC-010 Create Draft Order

Input:
- customer_id
- items[]

Steps:
1. Validate input
2. Compute subtotal/total
3. Insert Order (Draft)
4. Insert OrderItems

Errors:
CustomerNotFound
InvalidItem

Transaction: YES

---

## UC-020 Update Draft Order

Precondition:
Order.status == Draft

Steps:
- Load
- Apply changes
- Recompute totals
- Update

Transaction: YES

---

## UC-030 Confirm Order

Precondition:
Draft

Steps:
1. Check stock
2. status -> Confirmed
3. Trừ tồn kho (default policy)
4. Append DebtLedger

Errors:
InsufficientStock

Transaction: YES

---

## UC-040 Void Order

Steps:
1. Load order
2. Hoàn tồn kho (nếu đã confirm)
3. Ledger đảo
4. status -> Voided

Transaction: YES

---

## UC-050 Apply Payment

Input:
order_id
amount_vnd

Steps:
1. Insert Payment
2. Update paid_vnd
3. Update balance_vnd
4. status -> PartiallyPaid/Paid
5. Ledger delta = -amount

Errors:
OverPaymentNotAllowed

Transaction: YES

---

## UC-070 CRUD Customer/Product

- Create
- Update
- Soft delete
- Search

---

## UC-080 Reports

- Sales summary
- Debt report
- Inventory report

Rule:
- Run background thread