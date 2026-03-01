# Domain And Core

## 1) Money

- `core::Money` = `qint64`
- Dùng cho toàn bộ số tiền VND.
- Không dùng `double` cho nghiệp vụ tài chính.

## 2) Domain entities (`domain/entities.h`)

- `Customer`
  - `id`, `code`, `name`, `phone`, `address`, `creditLimitVnd`

- `Product`
  - `id`, `sku`, `name`, `unit`, `defaultWholesalePriceVnd`, `isActive`

- `ProductLot`
  - `id`, `productId`, `lotNo`, `receivedDate`, `expiryDate`, `unitCostVnd`, `onHandQty`

- `Inventory`
  - `productId`, `onHandQty` (struct sẵn có, chưa có service riêng)

- `InventoryMovement`
  - `id`, `productId`, `lotId`, `lotNo`
  - `movementType`, `reason`, `movementDate`
  - `qtyDelta`, `lotBalanceAfter`
  - dùng để lưu lịch sử tăng/giảm tồn theo lô

- `OrderItem`
  - `id`, `productId`, `qty`, `unitPriceVnd`, `discountVnd`, `lineTotalVnd`

- `Order`
  - `id`, `orderNo`, `customerId`, `orderDate`, `status`
  - `subtotalVnd`, `discountVnd`, `totalVnd`, `paidVnd`, `balanceVnd`

- `Payment`, `DebtLedger`
  - đã được dùng cho payment flow in-memory:
    - `Payment`: phiếu thu
    - `DebtLedger`: bút toán đối soát công nợ theo khách

- `DashboardMetrics`
  - `customerCount`, `productCount`, `openOrderCount`, `lowStockCount`, `expiringSoonProductCount`, `receivableVnd`

## 3) Order status enum

`OrderStatus` gồm:
- `Draft`
- `Confirmed`
- `PartiallyPaid`
- `Paid`
- `Voided`

Hiện tại luồng đã dùng thực tế: `Draft`, `Confirmed`, `PartiallyPaid`, `Paid`, `Voided`.
