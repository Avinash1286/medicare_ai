#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>
#include <QList>
#include <QVariant>

class DatabaseManager
{
public:
    explicit DatabaseManager();

    // Initializes the database, creates tables if they don't exist
    bool initDatabase();

    // Adds a new medicine to the database
    bool addMedicine(const QString& name, const QString& batchNumber, const QString& expiryDate, int quantity, double price);
    QList<QVariantList> getAllMedicines();

    qint64 createInvoice(double totalAmount, const QList<QPair<int, int>>& cartItems);

    bool updateMedicineQuantity(int medicineId, int quantityToSubtract);

    bool updateMedicine(int id, const QString& name, const QString& batch, const QString& expiry, int qty, double price);
    bool addStock(int id, int quantityToAdd);
    bool deleteMedicine(int id);
    QList<QVariantList> getInvoices();
    QList<QVariantList> getInvoiceDetails(qint64 invoiceId);

private:
    QSqlDatabase m_db;
    static const QString DB_PATH; // Store the database path as a constant
};

#endif // DATABASEMANAGER_H
