#pragma once

#include <QHash>
#include <QString>
#include <QVector>

#include "stockmaster/domain/entities.h"

namespace stockmaster::infra::db {
class DatabaseService;
}

namespace stockmaster::application {

class CustomerService;
class ProductService;

struct DraftOrderItemInput {
    QString productId;
    int qty = 0;
    core::Money unitPriceVnd = 0;
    core::Money discountVnd = 0;
};

class OrderService {
public:
    OrderService(const infra::db::DatabaseService &databaseService,
                 const CustomerService &customerService,
                 ProductService &productService);

    [[nodiscard]] domain::DashboardMetrics loadDashboardMetrics() const;

    [[nodiscard]] QVector<domain::Order> findOrders() const;
    [[nodiscard]] QVector<domain::OrderItem> findOrderItems(const QString &orderId) const;
    [[nodiscard]] bool getOrderById(const QString &orderId, domain::Order &order) const;
    [[nodiscard]] int openOrderCount() const;

    [[nodiscard]] bool createDraftOrder(const QString &customerId,
                                        const QString &orderDate,
                                        QString &orderIdOut,
                                        QString &errorMessage);
    [[nodiscard]] bool updateDraftOrderCustomer(const QString &orderId,
                                                const QString &customerId,
                                                QString &errorMessage);
    [[nodiscard]] bool upsertDraftItem(const QString &orderId,
                                       const DraftOrderItemInput &input,
                                       QString &errorMessage);
    [[nodiscard]] bool removeDraftItem(const QString &orderId,
                                       const QString &orderItemId,
                                       QString &errorMessage);
    [[nodiscard]] bool confirmOrder(const QString &orderId, QString &errorMessage);
    [[nodiscard]] bool applyPayment(const QString &orderId,
                                    core::Money amountVnd,
                                    QString &errorMessage);
    [[nodiscard]] bool voidOrder(const QString &orderId, QString &errorMessage);

private:
    struct StockAllocation {
        QString orderId;
        QString orderItemId;
        QString lotId;
        int qty = 0;
    };

    [[nodiscard]] bool findOrderIndex(const QString &orderId, qsizetype &index) const;
    [[nodiscard]] bool canEditDraft(const domain::Order &order, QString &errorMessage) const;
    [[nodiscard]] bool canVoid(const domain::Order &order, QString &errorMessage) const;
    [[nodiscard]] bool ensureCustomerExists(const QString &customerId) const;
    [[nodiscard]] bool ensureProductExists(const QString &productId) const;

    [[nodiscard]] bool planStockAllocations(const QString &orderId,
                                            const QVector<domain::OrderItem> &items,
                                            QVector<StockAllocation> &planned,
                                            QString &errorMessage) const;

    static core::Money calculateLineSubtotal(const domain::OrderItem &item);
    void recomputeOrderTotals(domain::Order &order);
    QString nextOrderNo();

    const infra::db::DatabaseService &m_databaseService;
    const CustomerService &m_customerService;
    ProductService &m_productService;

    QVector<domain::Order> m_orders;
    QHash<QString, QVector<domain::OrderItem>> m_orderItemsByOrder;
    QVector<StockAllocation> m_stockAllocations;

    int m_nextOrderSerial = 1;
};

} // namespace stockmaster::application
