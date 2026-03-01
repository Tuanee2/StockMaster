# Data Model — StockMaster

Rule quan trọng:
- Money dùng int64 (VND)
- Không dùng double

---

## Common Fields

id
created_at
updated_at
deleted_at

---

## Customer

id
code
name (required)
phone
address
note
credit_limit_vnd

---

## Product

id
sku
name (required)
unit
default_wholesale_price_vnd
barcode
is_active

---

## Inventory

product_id
on_hand_qty
updated_at

Rule:
- Không cho tồn âm (default)

---

## Order

id
order_no
customer_id
status
order_date
subtotal_vnd
discount_vnd
total_vnd
paid_vnd
balance_vnd
note

Status:

Draft
Confirmed
PartiallyPaid
Paid
Voided

---

## OrderItem

id
order_id
product_id
qty
unit_price_vnd
discount_vnd
line_total_vnd

---

## Payment

id
payment_no
customer_id
order_id (nullable)
amount_vnd
method
paid_at
note

---

## DebtLedger

id
customer_id
ref_type
ref_id
delta_vnd
balance_after_vnd
created_at

---

## Relationships

Customer 1-N Order  
Order 1-N OrderItem  
Customer 1-N Payment  
Product 1-1 Inventory

---

## Derived Values

Order.subtotal = sum(qty * price)
Order.total = subtotal - discount
Order.balance = total - paid