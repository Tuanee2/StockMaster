#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVariantList>
#include <QVector>

#include "stockmaster/application/order_service.h"

namespace stockmaster::application {
class CustomerService;
class ProductService;
}

namespace stockmaster::ui::viewmodels {

class OrdersViewModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(int orderCount READ orderCount NOTIFY orderCountChanged)
    Q_PROPERTY(QString queryOrderNo READ queryOrderNo NOTIFY queryChanged)
    Q_PROPERTY(QString queryFromDate READ queryFromDate NOTIFY queryChanged)
    Q_PROPERTY(QString queryToDate READ queryToDate NOTIFY queryChanged)
    Q_PROPERTY(bool hasSelectedOrder READ hasSelectedOrder NOTIFY selectedOrderChanged)
    Q_PROPERTY(QString selectedOrderId READ selectedOrderId NOTIFY selectedOrderChanged)
    Q_PROPERTY(QString selectedOrderNo READ selectedOrderNo NOTIFY selectedOrderChanged)
    Q_PROPERTY(QString selectedOrderDate READ selectedOrderDate NOTIFY selectedOrderChanged)
    Q_PROPERTY(QString selectedOrderStatus READ selectedOrderStatus NOTIFY selectedOrderChanged)
    Q_PROPERTY(QString selectedOrderCustomerId READ selectedOrderCustomerId NOTIFY selectedOrderChanged)
    Q_PROPERTY(QString selectedOrderTotalText READ selectedOrderTotalText NOTIFY selectedOrderChanged)
    Q_PROPERTY(bool canEditDraft READ canEditDraft NOTIFY selectedOrderChanged)
    Q_PROPERTY(bool canConfirm READ canConfirm NOTIFY selectedOrderChanged)
    Q_PROPERTY(bool canVoid READ canVoid NOTIFY selectedOrderChanged)
    Q_PROPERTY(bool canOpenPayment READ canOpenPayment NOTIFY selectedOrderChanged)
    Q_PROPERTY(QVariantList selectedOrderItems READ selectedOrderItems NOTIFY selectedOrderItemsChanged)
    Q_PROPERTY(QVariantList customers READ customers NOTIFY lookupChanged)
    Q_PROPERTY(QVariantList products READ products NOTIFY lookupChanged)

public:
    enum OrderRole {
        OrderIdRole = Qt::UserRole + 1,
        OrderNoRole,
        OrderDateRole,
        StatusRole,
        CustomerNameRole,
        TotalTextRole,
        ItemCountRole,
    };

    OrdersViewModel(application::OrderService &orderService,
                    const application::CustomerService &customerService,
                    const application::ProductService &productService,
                    QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString lastError() const;
    [[nodiscard]] int orderCount() const;
    [[nodiscard]] QString queryOrderNo() const;
    [[nodiscard]] QString queryFromDate() const;
    [[nodiscard]] QString queryToDate() const;

    [[nodiscard]] bool hasSelectedOrder() const;
    [[nodiscard]] QString selectedOrderId() const;
    [[nodiscard]] QString selectedOrderNo() const;
    [[nodiscard]] QString selectedOrderDate() const;
    [[nodiscard]] QString selectedOrderStatus() const;
    [[nodiscard]] QString selectedOrderCustomerId() const;
    [[nodiscard]] QString selectedOrderTotalText() const;

    [[nodiscard]] bool canEditDraft() const;
    [[nodiscard]] bool canConfirm() const;
    [[nodiscard]] bool canVoid() const;
    [[nodiscard]] bool canOpenPayment() const;

    [[nodiscard]] QVariantList selectedOrderItems() const;
    [[nodiscard]] QVariantList customers() const;
    [[nodiscard]] QVariantList products() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool createDraftOrder(const QString &customerId, const QString &orderDate);
    Q_INVOKABLE void selectOrder(const QString &orderId);
    Q_INVOKABLE bool updateSelectedOrderCustomer(const QString &customerId);

    Q_INVOKABLE bool addOrUpdateDraftItem(const QString &productId,
                                          const QString &preferredLotId,
                                          const QString &qtyText,
                                          const QString &unitPriceText,
                                          const QString &discountText);
    Q_INVOKABLE bool removeDraftItem(const QString &orderItemId);

    Q_INVOKABLE bool confirmSelectedOrder();
    Q_INVOKABLE bool voidSelectedOrder();

    Q_INVOKABLE QString defaultPriceTextForProduct(const QString &productId) const;
    Q_INVOKABLE QVariantList lotsForProduct(const QString &productId) const;
    Q_INVOKABLE QVariantList previewAllocations(const QString &productId,
                                                const QString &preferredLotId,
                                                const QString &qtyText) const;
    Q_INVOKABLE QString orderIdAt(int row) const;
    Q_INVOKABLE bool applyOrderQuery(const QString &orderNo,
                                     const QString &fromDate,
                                     const QString &toDate);
    Q_INVOKABLE void clearOrderQuery();

signals:
    void lastErrorChanged();
    void orderCountChanged();
    void selectedOrderChanged();
    void selectedOrderItemsChanged();
    void lookupChanged();
    void queryChanged();

private:
    [[nodiscard]] static QString statusText(domain::OrderStatus status);
    [[nodiscard]] static QString formatMoneyVnd(core::Money amountVnd);
    [[nodiscard]] bool parseMoneyInput(const QString &input, core::Money &value) const;
    [[nodiscard]] bool parsePositiveInt(const QString &input, int &value) const;
    [[nodiscard]] bool selectedOrderInStatus(domain::OrderStatus status) const;
    [[nodiscard]] QVariantList buildAllocationPreview(const QString &productId,
                                                      const QString &preferredLotId,
                                                      int qty) const;
    [[nodiscard]] bool validateAndNormalizeQuery(const QString &orderNo,
                                                 const QString &fromDate,
                                                 const QString &toDate,
                                                 QString &normalizedOrderNo,
                                                 QString &normalizedFromDate,
                                                 QString &normalizedToDate,
                                                 QString &errorMessage) const;
    void rebuildFilteredOrders();

    void setLastError(const QString &message);
    void refreshOrders();
    void refreshSelectedOrder();
    void refreshLookups();
    void clearSelection();

    application::OrderService &m_orderService;
    const application::CustomerService &m_customerService;
    const application::ProductService &m_productService;

    QVector<domain::Order> m_allOrders;
    QVector<domain::Order> m_orders;
    QString m_queryOrderNo;
    QString m_queryFromDate;
    QString m_queryToDate;

    bool m_hasSelectedOrder = false;
    domain::Order m_selectedOrder;

    QString m_lastError;
    QVariantList m_selectedOrderItems;
    QVariantList m_customers;
    QVariantList m_products;
};

} // namespace stockmaster::ui::viewmodels
