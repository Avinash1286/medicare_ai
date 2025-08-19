#ifndef SALESHISTORYDIALOG_H
#define SALESHISTORYDIALOG_H

#include <QDialog>
#include "databasemanager.h" // Essential for data access

// Forward declarations for UI elements to keep header clean
class QTableWidget;
class QGroupBox;

class SalesHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    // Constructor that takes a pointer to the database manager
    explicit SalesHistoryDialog(DatabaseManager *dbManager, QWidget *parent = nullptr);

private slots:
    // Slot to be triggered when a user clicks on an invoice in the left table
    void onInvoiceSelected(int row, int column);

private:
    // Helper function to set up the entire UI for this dialog
    void setupUI();
    // Helper function to load all invoices from the DB into the left table
    void populateInvoicesTable();

    DatabaseManager *m_dbManager;      // Pointer to the main database manager
    QTableWidget *m_invoicesTable;     // Table to display the list of all invoices
    QTableWidget *m_detailsTable;      // Table to display items for a selected invoice
};

#endif // SALESHISTORYDIALOG_H
