#pragma once

#include <QString>

#include "stockmaster/core/money.h"

namespace stockmaster::domain {

enum class OrderStatus {
    Draft,
    Confirmed,
    PartiallyPaid,
    Paid,
    Voided
};

struct Customer {
    QString id;
    QString code;
    QString name;
    QString phone;
    QString address;
    core::Money creditLimitVnd = 0;
};

struct Product {
    QString id;
    QString sku;
    QString name;
    QString unit;
    core::Money defaultWholesalePriceVnd = 0;
    bool isActive = true;
};

struct ProductLot {
    QString id;
    QString productId;
    QString lotNo;
    QString receivedDate;
    QString expiryDate;
    core::Money unitCostVnd = 0;
    int onHandQty = 0;
};

struct Inventory {
    QString productId;
    int onHandQty = 0;
};

struct InventoryMovement {
    QString id;
    QString productId;
    QString lotId;
    QString lotNo;
    QString movementType;
    QString reason;
    QString movementDate;
    int qtyDelta = 0;
    int lotBalanceAfter = 0;
};

struct OrderItem {
    QString id;
    QString productId;
    int qty = 0;
    core::Money unitPriceVnd = 0;
    core::Money discountVnd = 0;
    core::Money lineTotalVnd = 0;
};

struct Order {
    QString id;
    QString orderNo;
    QString customerId;
    QString orderDate;
    OrderStatus status = OrderStatus::Draft;
    core::Money subtotalVnd = 0;
    core::Money discountVnd = 0;
    core::Money totalVnd = 0;
    core::Money paidVnd = 0;
    core::Money balanceVnd = 0;
};

struct Payment {
    QString id;
    QString paymentNo;
    QString customerId;
    QString orderId;
    core::Money amountVnd = 0;
    QString method;
    QString paidAt;
};

struct DebtLedger {
    QString id;
    QString customerId;
    QString refType;
    QString refId;
    core::Money deltaVnd = 0;
    core::Money balanceAfterVnd = 0;
    QString createdAt;
};

struct DashboardMetrics {
    int customerCount = 0;
    int productCount = 0;
    int openOrderCount = 0;
    int lowStockCount = 0;
    int expiringSoonProductCount = 0;
    core::Money receivableVnd = 0;
};

} // namespace stockmaster::domain
