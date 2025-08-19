#include "saleshistorydialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDebug>

SalesHistoryDialog::SalesHistoryDialog(DatabaseManager *dbManager, QWidget *parent)
    : QDialog(parent), m_dbManager(dbManager)
{
    setWindowTitle("Sales History & Invoice Details");
    setMinimumSize(800, 600);
    setupUI();

    populateInvoicesTable();

    connect(m_invoicesTable, &QTableWidget::cellClicked, this, &SalesHistoryDialog::onInvoiceSelected);

    if (m_invoicesTable->rowCount() > 0) {
        m_invoicesTable->selectRow(0);
        onInvoiceSelected(0, 0);
    }
}

void SalesHistoryDialog::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QGroupBox *invoicesGroup = new QGroupBox("Invoices");
    QVBoxLayout *invoicesLayout = new QVBoxLayout();

    m_invoicesTable = new QTableWidget(this);
    m_invoicesTable->setColumnCount(3);
    m_invoicesTable->setHorizontalHeaderLabels({"ID", "Date of Sale", "Total Amount"});
    m_invoicesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_invoicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_invoicesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_invoicesTable->verticalHeader()->setVisible(false);
    m_invoicesTable->setColumnHidden(0, true);
    m_invoicesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    invoicesLayout->addWidget(m_invoicesTable);
    invoicesGroup->setLayout(invoicesLayout);
    QGroupBox *detailsGroup = new QGroupBox("Invoice Details");
    QVBoxLayout *detailsLayout = new QVBoxLayout();

    m_detailsTable = new QTableWidget(this);
    m_detailsTable->setColumnCount(3);
    m_detailsTable->setHorizontalHeaderLabels({"Medicine Name", "Quantity Sold", "Price at Sale"});
    m_detailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_detailsTable->verticalHeader()->setVisible(false);
    m_detailsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    detailsLayout->addWidget(m_detailsTable);
    detailsGroup->setLayout(detailsLayout);
    mainLayout->addWidget(invoicesGroup, 1);
    mainLayout->addWidget(detailsGroup, 2);

    this->setLayout(mainLayout);
}

void SalesHistoryDialog::populateInvoicesTable()
{
    if (!m_dbManager) {
        qWarning() << "DatabaseManager not available in SalesHistoryDialog.";
        return;
    }

    m_invoicesTable->setRowCount(0);

    QList<QVariantList> invoices = m_dbManager->getInvoices();
    m_invoicesTable->setRowCount(invoices.count());

    for (int i = 0; i < invoices.count(); ++i) {
        const QVariantList& invoiceData = invoices.at(i);

        QTableWidgetItem *idItem = new QTableWidgetItem(invoiceData[0].toString());
        idItem->setData(Qt::UserRole, invoiceData[0]);
        m_invoicesTable->setItem(i, 0, idItem);

        // Column 1: Date
        QTableWidgetItem *dateItem = new QTableWidgetItem(invoiceData[1].toString());
        m_invoicesTable->setItem(i, 1, dateItem);
        double total = invoiceData[2].toDouble();
        QTableWidgetItem *totalItem = new QTableWidgetItem(QString::number(total, 'f', 2));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_invoicesTable->setItem(i, 2, totalItem);
    }
}

void SalesHistoryDialog::onInvoiceSelected(int row, int column)
{
    Q_UNUSED(column);
    QTableWidgetItem *idItem = m_invoicesTable->item(row, 0);
    if (!idItem) return;

    qint64 invoiceId = idItem->data(Qt::UserRole).toLongLong();

    m_detailsTable->setRowCount(0);

    QList<QVariantList> details = m_dbManager->getInvoiceDetails(invoiceId);
    m_detailsTable->setRowCount(details.count());

    for (int i = 0; i < details.count(); ++i) {
        const QVariantList& detailData = details.at(i);

        // Column 0: Medicine Name
        QTableWidgetItem *nameItem = new QTableWidgetItem(detailData[0].toString());
        m_detailsTable->setItem(i, 0, nameItem);

        // Column 1: Quantity Sold
        QTableWidgetItem *qtyItem = new QTableWidgetItem(detailData[1].toString());
        qtyItem->setTextAlignment(Qt::AlignCenter);
        m_detailsTable->setItem(i, 1, qtyItem);

        // Column 2: Price at Sale (formatted as currency)
        double price = detailData[2].toDouble();
        QTableWidgetItem *priceItem = new QTableWidgetItem(QString::number(price, 'f', 2));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_detailsTable->setItem(i, 2, priceItem);
    }
}
