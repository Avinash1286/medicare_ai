#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>

// Define the static constant for the database path
const QString DatabaseManager::DB_PATH = "database/medicare.db";

// In databasemanager.cpp

#include <QFileInfo> // <-- Add this include at the top

DatabaseManager::DatabaseManager()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");

    // Let's debug the pathing very carefully
    QString dbFolderPath = "database";
    QString dbFileName = "medicare.db";
    QString finalDbPath = dbFolderPath + "/" + dbFileName;

    qDebug() << "Attempting to create database in relative path:" << finalDbPath;

    QDir dir(dbFolderPath);
    if (!dir.exists()) {
        qDebug() << "Database directory does not exist, creating it at:" << dir.absolutePath();
        if (!dir.mkpath(".")) {
            qDebug() << "CRITICAL: Could not create database directory!";
            // The application will likely fail after this, which is what we want to see.
        }
    } else {
        qDebug() << "Database directory already exists at:" << dir.absolutePath();
    }

    // Now, set the full, absolute path for the database name
    m_db.setDatabaseName(dir.absoluteFilePath(dbFileName));

    qDebug() << "Final absolute database path is:" << m_db.databaseName();
}
bool DatabaseManager::initDatabase()
{
    if (!m_db.open()) {
        qDebug() << "Error: connection with database failed:" << m_db.lastError().text();
        return false;
    }

    qDebug() << "Database: connection ok";

    QSqlQuery query;

    // Create Medicines Table
    bool success = query.exec("CREATE TABLE IF NOT EXISTS Medicines ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "name TEXT NOT NULL, "
                              "batchNumber TEXT, "
                              "expiryDate TEXT, "
                              "quantity INTEGER, "
                              "price REAL"
                              ");");
    if(!success) {
        qDebug() << "Failed to create Medicines table:" << query.lastError().text();
        return false;
    }

    // Create Invoices Table
    success = query.exec("CREATE TABLE IF NOT EXISTS Invoices ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "saleDate TEXT, "
                         "totalAmount REAL"
                         ");");
    if(!success) {
        qDebug() << "Failed to create Invoices table:" << query.lastError().text();
        return false;
    }

    // Create InvoiceItems Table
    success = query.exec("CREATE TABLE IF NOT EXISTS InvoiceItems ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "invoiceId INTEGER, "
                         "medicineId INTEGER, "
                         "quantitySold INTEGER, "
                         "priceAtSale REAL, "
                         "FOREIGN KEY(invoiceId) REFERENCES Invoices(id), "
                         "FOREIGN KEY(medicineId) REFERENCES Medicines(id)"
                         ");");
    if(!success) {
        qDebug() << "Failed to create InvoiceItems table:" << query.lastError().text();
        return false;
    }

    qDebug() << "All tables created or already exist.";
    return true;
}

bool DatabaseManager::addMedicine(const QString& name, const QString& batchNumber, const QString& expiryDate, int quantity, double price)
{
    QSqlQuery query;
    query.prepare("INSERT INTO Medicines (name, batchNumber, expiryDate, quantity, price) "
                  "VALUES (:name, :batchNumber, :expiryDate, :quantity, :price)");
    query.bindValue(":name", name);
    query.bindValue(":batchNumber", batchNumber);
    query.bindValue(":expiryDate", expiryDate);
    query.bindValue(":quantity", quantity);
    query.bindValue(":price", price);

    if (query.exec()) {
        qDebug() << "Successfully added medicine:" << name;
        return true;
    } else {
        qDebug() << "Failed to add medicine:" << query.lastError().text();
        return false;
    }
}


QList<QVariantList> DatabaseManager::getAllMedicines()
{
    QList<QVariantList> medicines;
    QSqlQuery query("SELECT id, name, batchNumber, expiryDate, quantity, price FROM Medicines");

    if (!query.exec()) {
        qDebug() << "Failed to fetch medicines:" << query.lastError().text();
        return medicines; // Return empty list on failure
    }

    while (query.next()) {
        QVariantList row;
        row << query.value(0); // ID
        row << query.value(1); // Name
        row << query.value(2); // Batch Number
        row << query.value(3); // Expiry Date
        row << query.value(4); // Quantity
        row << query.value(5); // Price
        medicines.append(row);
    }

    return medicines;
}


bool DatabaseManager::updateMedicineQuantity(int medicineId, int quantityToSubtract)
{
    QSqlQuery query;
    query.prepare("UPDATE Medicines SET quantity = quantity - :quantityToSubtract WHERE id = :id");
    query.bindValue(":quantityToSubtract", quantityToSubtract);
    query.bindValue(":id", medicineId);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to update quantity for medicine ID" << medicineId << ":" << query.lastError().text();
        return false;
    }
}

qint64 DatabaseManager::createInvoice(double totalAmount, const QList<QPair<int, int>>& cartItems)
{
    // A transaction ensures that all queries succeed or none do.
    // This prevents a partial sale from being recorded if one query fails.
    if (!m_db.transaction()) {
        qDebug() << "Failed to start transaction:" << m_db.lastError().text();
        return -1;
    }

    // 1. Create the Invoice record
    QSqlQuery invoiceQuery;
    invoiceQuery.prepare("INSERT INTO Invoices (saleDate, totalAmount) VALUES (:date, :total)");
    invoiceQuery.bindValue(":date", QDateTime::currentDateTime().toString(Qt::ISODate));
    invoiceQuery.bindValue(":total", totalAmount);

    if (!invoiceQuery.exec()) {
        qDebug() << "Failed to create invoice:" << invoiceQuery.lastError().text();
        m_db.rollback();
        return -1;
    }
    qint64 invoiceId = invoiceQuery.lastInsertId().toLongLong();

    // 2. Create InvoiceItems records and update stock
    for (const auto& item : cartItems) {
        int medicineId = item.first;
        int quantitySold = item.second;

        // Get current price
        QSqlQuery priceQuery;
        priceQuery.prepare("SELECT price FROM Medicines WHERE id = :id");
        priceQuery.bindValue(":id", medicineId);
        priceQuery.exec();
        double priceAtSale = 0.0;
        if (priceQuery.next()) {
            priceAtSale = priceQuery.value(0).toDouble();
        }

        // Insert into InvoiceItems
        QSqlQuery itemQuery;
        itemQuery.prepare("INSERT INTO InvoiceItems (invoiceId, medicineId, quantitySold, priceAtSale) "
                          "VALUES (:invoiceId, :medicineId, :quantity, :price)");
        itemQuery.bindValue(":invoiceId", invoiceId);
        itemQuery.bindValue(":medicineId", medicineId);
        itemQuery.bindValue(":quantity", quantitySold);
        itemQuery.bindValue(":price", priceAtSale);

        if (!itemQuery.exec()) {
            qDebug() << "Failed to add item to invoice:" << itemQuery.lastError().text();
            m_db.rollback();
            return -1;
        }

        // Update the stock quantity
        if (!updateMedicineQuantity(medicineId, quantitySold)) {
            m_db.rollback();
            return -1;
        }
    }

    // If everything succeeded, commit the transaction
    if (!m_db.commit()) {
        qDebug() << "Failed to commit transaction:" << m_db.lastError().text();
        return -1;
    }

    return invoiceId;
}


bool DatabaseManager::updateMedicine(int id, const QString& name, const QString& batch, const QString& expiry, int qty, double price)
{
    QSqlQuery query;
    query.prepare("UPDATE Medicines SET name = :name, batchNumber = :batch, "
                  "expiryDate = :expiry, quantity = :qty, price = :price WHERE id = :id");
    query.bindValue(":name", name);
    query.bindValue(":batch", batch);
    query.bindValue(":expiry", expiry);
    query.bindValue(":qty", qty);
    query.bindValue(":price", price);
    query.bindValue(":id", id);
    if(query.exec()) return true;
    qDebug() << "Failed to update medicine:" << query.lastError();
    return false;
}

bool DatabaseManager::addStock(int id, int quantityToAdd)
{
    QSqlQuery query;
    query.prepare("UPDATE Medicines SET quantity = quantity + :qty WHERE id = :id");
    query.bindValue(":qty", quantityToAdd);
    query.bindValue(":id", id);
    if(query.exec()) return true;
    qDebug() << "Failed to add stock:" << query.lastError();
    return false;
}

QList<QVariantList> DatabaseManager::getInvoices()
{
    QList<QVariantList> invoices;
    QSqlQuery query("SELECT id, saleDate, totalAmount FROM Invoices ORDER BY id DESC");
    while (query.next()) {
        invoices.append({query.value(0), query.value(1), query.value(2)});
    }
    return invoices;
}

QList<QVariantList> DatabaseManager::getInvoiceDetails(qint64 invoiceId)
{
    QList<QVariantList> details;
    QSqlQuery query;
    query.prepare("SELECT m.name, i.quantitySold, i.priceAtSale "
                  "FROM InvoiceItems i JOIN Medicines m ON i.medicineId = m.id "
                  "WHERE i.invoiceId = :id");
    query.bindValue(":id", invoiceId);
    query.exec();
    while (query.next()) {
        details.append({query.value(0), query.value(1), query.value(2)});
    }
    return details;
}


bool DatabaseManager::deleteMedicine(int id)
{
    // Important: Prevent deletion if the medicine is part of any past sale
    // This maintains data integrity.
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM InvoiceItems WHERE medicineId = :id");
    checkQuery.bindValue(":id", id);
    if (checkQuery.exec() && checkQuery.next()) {
        if (checkQuery.value(0).toInt() > 0) {
            qDebug() << "Cannot delete medicine ID" << id << "as it is part of existing invoices.";
            return false; // Deletion failed because it's in use
        }
    }

    // If it's not in any invoices, proceed with deletion
    QSqlQuery deleteQuery;
    deleteQuery.prepare("DELETE FROM Medicines WHERE id = :id");
    deleteQuery.bindValue(":id", id);

    if (deleteQuery.exec()) {
        qDebug() << "Successfully deleted medicine ID" << id;
        return true;
    } else {
        qDebug() << "Failed to delete medicine:" << deleteQuery.lastError().text();
        return false;
    }
}
