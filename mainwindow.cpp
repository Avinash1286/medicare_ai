#include "mainwindow.h"
#include "addmedicinedialog.h"
#include "saleshistorydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QTableWidget>
#include <QHeaderView>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->setWindowTitle("Pharma Copilot");
    this->setMinimumSize(500, 500);
    this->resize(1400, 900);

    // Apply the modern stylesheet directly
    this->setStyleSheet(getModernStyleSheet());

    // Initialize Backend
    m_dbManager = new DatabaseManager();
    if (!m_dbManager->initDatabase()) {
        QMessageBox::critical(this, "Database Error", "Failed to initialize the database.");
    }

    // Initialize Networking for PharmaCopilot
     m_networkManager = new QNetworkAccessManager(this);
     connect(m_networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onGeminiReplyFinished);

    // Build the UI and populate it with data
    setupModernUI();
    populateStockTable();
}

MainWindow::~MainWindow()
{
    delete m_dbManager;
}

void MainWindow::setupModernUI()
{
    QWidget *centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // --- Left Side: Inventory Management ---
    QFrame *inventoryFrame = createModernFrame();
    QVBoxLayout *inventoryLayout = new QVBoxLayout(inventoryFrame);
    inventoryLayout->setSpacing(15);

    QLabel *inventoryHeader = new QLabel("📦 Inventory Management");
    inventoryHeader->setProperty("labelType", "header");

    m_searchLineEdit = new QLineEdit();
    m_searchLineEdit->setPlaceholderText("Search medicines by name...");
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchQueryChanged);

    QHBoxLayout *buttonToolbar = new QHBoxLayout();
    buttonToolbar->setSpacing(12);
    ModernButton *addButton = new ModernButton("➕ Add Medicine");
    addButton->setButtonType("primary");
    connect(addButton, &QPushButton::clicked, this, &MainWindow::onAddMedicineClicked);
    ModernButton *historyButton = new ModernButton("📊 Sales History");
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::onSalesHistoryClicked);
    buttonToolbar->addWidget(addButton);
    buttonToolbar->addWidget(historyButton);
    buttonToolbar->addStretch();

    m_stockTableWidget = new QTableWidget();
    setupModernTable();

    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(15);
    m_totalStatsCard = new StatsCard("Total Medicines", "📊", QColor("#667eea"));
    m_lowStockCard = new StatsCard("Low Stock", "⚠️", QColor("#f6ad55"));
    m_expiringCard = new StatsCard("Expiring Soon", "⏰", QColor("#f56565"));
    statsLayout->addWidget(m_totalStatsCard);
    statsLayout->addWidget(m_lowStockCard);
    statsLayout->addWidget(m_expiringCard);
    statsLayout->addStretch();

    inventoryLayout->addWidget(inventoryHeader);
    inventoryLayout->addWidget(m_searchLineEdit);
    inventoryLayout->addLayout(buttonToolbar);
    inventoryLayout->addWidget(m_stockTableWidget);
    inventoryLayout->addLayout(statsLayout);

    // --- Right Side: POS & AI Assistant ---
    QFrame *rightFrame = createModernFrame();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightFrame);
    rightLayout->setSpacing(15);

    // PharmaCopilot Section
    QLabel *copilotHeader = new QLabel("🤖 PharmaCopilot AI");
    copilotHeader->setProperty("labelType", "header");
    m_symptomsLineEdit = new QLineEdit();
    m_symptomsLineEdit->setPlaceholderText("Enter symptoms (e.g., cough, fever)...");
    // --- MODIFICATION START ---
    // Create a horizontal layout for the Copilot buttons
    QHBoxLayout *copilotButtonsLayout = new QHBoxLayout();
    copilotButtonsLayout->setSpacing(10);

    // The "Ask" button remains the same
    m_askCopilotButton = new ModernButton("Ask Copilot");
    connect(m_askCopilotButton, &QPushButton::clicked, this, &MainWindow::onAskCopilotClicked);

    // Create the new "Clear" button
    ModernButton *clearCopilotButton = new ModernButton("❌ Clear");
    clearCopilotButton->setButtonType("secondary"); // "danger" is also a good option
    connect(clearCopilotButton, &QPushButton::clicked, this, &MainWindow::onClearCopilotClicked);

    // Add both buttons to the new layout
    copilotButtonsLayout->addWidget(m_askCopilotButton, 2); // Give "Ask" more stretch space
    copilotButtonsLayout->addWidget(clearCopilotButton, 1);

    m_suggestionsListWidget = new QListWidget();
    m_suggestionsListWidget->setToolTip("Double-click a suggestion to add it to the cart.");
    connect(m_suggestionsListWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item){
        // Get the name of the medicine from the clicked suggestion item
        QString medicineName = item->text();

        // Loop through the main stock table to find the matching medicine
        for (int i = 0; i < m_stockTableWidget->rowCount(); ++i) {
            // Check if the row is visible (not filtered out by search)
            if (!m_stockTableWidget->isRowHidden(i)) {
                QTableWidgetItem* tableItem = m_stockTableWidget->item(i, 1); // Column 1 is "Medicine Name"
                if (tableItem && tableItem->text() == medicineName) {
                    // We found a match!
                    // Simulate a double-click on the main table to reuse all existing logic.
                    onStockTableDoubleClicked(i, 1);
                    return; // Exit the loop once the item is added
                }
            }
        }
    });
    // --- END OF FIX ---

    // Point of Sale Section
    QLabel *posHeader = new QLabel("🛒 Point of Sale");
    posHeader->setProperty("labelType", "header");
    m_cartListWidget = new QListWidget();
    m_totalAmountLabel = new QLabel("Total: $0.00");
    m_totalAmountLabel->setProperty("labelType", "total");
    m_totalAmountLabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *cartButtonsLayout = new QHBoxLayout();
    ModernButton *clearCartButton = new ModernButton("🗑️ Clear Cart");
    clearCartButton->setButtonType("danger");
    connect(clearCartButton, &QPushButton::clicked, this, &MainWindow::onClearCartClicked);
    ModernButton *finalizeSaleButton = new ModernButton("💳 Finalize Sale");
    finalizeSaleButton->setButtonType("primary");
    finalizeSaleButton->setFixedHeight(50);
    connect(finalizeSaleButton, &QPushButton::clicked, this, &MainWindow::onFinalizeSaleClicked);
    cartButtonsLayout->addWidget(clearCartButton);
    cartButtonsLayout->addWidget(finalizeSaleButton, 2);

    // Assemble Right Layout
    rightLayout->addWidget(copilotHeader);
    rightLayout->addWidget(m_symptomsLineEdit);
    rightLayout->addLayout(copilotButtonsLayout);
    rightLayout->addWidget(m_suggestionsListWidget, 1); // Give suggestions list stretch factor
    rightLayout->addWidget(posHeader);
    rightLayout->addWidget(m_cartListWidget, 2); // Give cart more stretch factor
    rightLayout->addWidget(m_totalAmountLabel);
    rightLayout->addLayout(cartButtonsLayout);

    // --- Final Assembly ---
    mainLayout->addWidget(inventoryFrame, 7);
    mainLayout->addWidget(rightFrame, 3);
}


QString MainWindow::getModernStyleSheet()
{
    // This function returns the large QSS string you provided
    // (Pasting the full string here for brevity is omitted, but it's the one from your prompt)
    return R"(
        QWidget {
            font-family: Segoe UI, sans-serif;
            color: #e2e8f0;
        }
        QMainWindow {
            background-color: #1a202c;
        }
        QFrame {
            background-color: #2d3748;
            border-radius: 15px;
        }
        QLabel[labelType="header"] {
            color: #a0aec0;
            font-size: 11pt;
            font-weight: bold;
            text-transform: uppercase;
            margin-bottom: 5px;
        }
        QLineEdit {
            background-color: #1a202c;
            border: 1px solid #4a5568;
            border-radius: 8px;
            padding: 10px;
            font-size: 11pt;
        }
        QLineEdit:focus {
            border: 1px solid #667eea;
        }
        QTableWidget {
            background-color: #2d3748;
            border: none;
            border-radius: 8px;
            gridline-color: #4a5568;
            font-size: 10pt;
            selection-background-color: #4c51bf;
        }
        QHeaderView::section {
            background-color: #1a202c;
            color: #a0aec0;
            padding: 10px;
            border: none;
            font-weight: bold;
        }
        QListWidget {
            background-color: #1a202c;
            border: 1px solid #4a5568;
            border-radius: 8px;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #4a5568;
        }
        QListWidget::item:selected {
            background-color: #4c51bf;
        }
        QLabel[labelType="total"] {
            color: #48bb78;
            font-size: 20pt;
            font-weight: bold;
        }
        /* ... Add more detailed QSS from your prompt if desired ... */
    )";
}

QFrame* MainWindow::createModernFrame()
{
    QFrame *frame = new QFrame();
    frame->setFrameStyle(QFrame::NoFrame);
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 5);
    frame->setGraphicsEffect(shadow);
    return frame;
}

void MainWindow::setupModernTable()
{
    m_stockTableWidget->setColumnCount(6);
    m_stockTableWidget->setHorizontalHeaderLabels({"ID", "Medicine Name", "Batch", "Expiry", "Qty", "Price"});

    // --- THIS IS THE CORRECT AND FINAL FIX ---
    // We explicitly tell the table to use a different trigger for editing,
    // which frees up the DoubleClick action to emit its signal properly.
    m_stockTableWidget->setEditTriggers(QAbstractItemView::SelectedClicked);
    // -----------------------------------------

    m_stockTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stockTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_stockTableWidget->setAlternatingRowColors(false); // This is often handled by modern stylesheets
    m_stockTableWidget->verticalHeader()->setVisible(false);
    m_stockTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_stockTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_stockTableWidget, &QTableWidget::customContextMenuRequested, this, &MainWindow::showTableContextMenu);

    // This connect statement is and always has been correct.
    // It will now work because the event is no longer blocked.
    connect(m_stockTableWidget, &QTableWidget::cellDoubleClicked, this, &MainWindow::onStockTableDoubleClicked);
}

void MainWindow::updateStockStats()
{
    if (!m_totalStatsCard) return;
    int totalMedicines = 0, lowStock = 0, expiringSoon = 0;
    int lowStockThreshold = 10;
    QDate today = QDate::currentDate();
    for (int i = 0; i < m_stockTableWidget->rowCount(); ++i) {
        if (!m_stockTableWidget->isRowHidden(i)) {
            totalMedicines++;
            int quantity = m_stockTableWidget->item(i, 4)->text().toInt();
            QDate expiryDate = QDate::fromString(m_stockTableWidget->item(i, 3)->text(), "yyyy-MM-dd");
            if (quantity < lowStockThreshold) lowStock++;
            if (expiryDate >= today && expiryDate < today.addMonths(3)) expiringSoon++;
        }
    }
    m_totalStatsCard->updateValue(QString::number(totalMedicines));
    m_lowStockCard->updateValue(QString::number(lowStock));
    m_expiringCard->updateValue(QString::number(expiringSoon));
}

void MainWindow::populateStockTable()
{
    if (!m_stockTableWidget) return;
    // Disconnecting during population can prevent unwanted signals and speed up large updates
    m_stockTableWidget->setSortingEnabled(false);
    m_stockTableWidget->clearContents();
    m_stockTableWidget->setRowCount(0);

    QList<QVariantList> medicines = m_dbManager->getAllMedicines();
    m_stockTableWidget->setRowCount(medicines.count());

    int lowStockThreshold = 10;
    QDate today = QDate::currentDate();

    for (int i = 0; i < medicines.count(); ++i) {
        const QVariantList& medicine = medicines.at(i);

        // --- Create items for each column ---
        QTableWidgetItem *idItem = new QTableWidgetItem(medicine[0].toString());
        QTableWidgetItem *nameItem = new QTableWidgetItem(medicine[1].toString());
        QTableWidgetItem *batchItem = new QTableWidgetItem(medicine[2].toString());
        QTableWidgetItem *expiryItem = new QTableWidgetItem(medicine[3].toString());
        QTableWidgetItem *quantityItem = new QTableWidgetItem(medicine[4].toString());
        QTableWidgetItem *priceItem = new QTableWidgetItem(QString::number(medicine[5].toDouble(), 'f', 2));

        // --- THIS IS THE FIX ---
        // Create a list of the new items to easily loop through them
        QList<QTableWidgetItem*> items = {idItem, nameItem, batchItem, expiryItem, quantityItem, priceItem};
        for (QTableWidgetItem *item : items) {
            // Get the default flags and remove the "ItemIsEditable" flag
            Qt::ItemFlags flags = item->flags();
            flags &= ~Qt::ItemIsEditable; // This is a bitwise operation to switch off the editable flag
            item->setFlags(flags);
        }
        // ----------------------

        // Now set the items in the table
        m_stockTableWidget->setItem(i, 0, idItem);
        m_stockTableWidget->setItem(i, 1, nameItem);
        m_stockTableWidget->setItem(i, 2, batchItem);
        m_stockTableWidget->setItem(i, 3, expiryItem);
        m_stockTableWidget->setItem(i, 4, quantityItem);
        m_stockTableWidget->setItem(i, 5, priceItem);

        // --- Visual Alerts (Your existing code is good) ---
        bool isLowStock = medicine[4].toInt() < lowStockThreshold;
        QDate expiryDate = QDate::fromString(medicine[3].toString(), "yyyy-MM-dd");
        bool isExpired = expiryDate < today;
        bool isExpiringSoon = expiryDate >= today && expiryDate < today.addMonths(3);

        QColor rowColor(0,0,0,0); // Transparent
        if (isExpired) rowColor = QColor(229, 62, 62, 50);
        else if (isLowStock) rowColor = QColor(246, 173, 85, 50);
        else if (isExpiringSoon) rowColor = QColor(236, 201, 75, 50);

        if (rowColor.alpha() > 0) {
            for (int j = 0; j < m_stockTableWidget->columnCount(); ++j) {
                m_stockTableWidget->item(i, j)->setBackground(rowColor);
            }
        }
    }
    m_stockTableWidget->setSortingEnabled(true);
    updateStockStats();
}

void MainWindow::onClearCartClicked()
{
    if (m_cartListWidget->count() == 0) return;
    auto reply = QMessageBox::question(this, "Clear Cart", "Are you sure you want to clear all items from the cart?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_cartListWidget->clear();
        updateTotalAmount();
    }
}

void MainWindow::onAddMedicineClicked()
{
    AddMedicineDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.name();
        if (name.isEmpty()) {
            QMessageBox::warning(this, "Input Error", "Medicine name cannot be empty.");
            return;
        }
        if (m_dbManager->addMedicine(name, dialog.batchNumber(), dialog.expiryDate(), dialog.quantity(), dialog.price())) {
            QMessageBox::information(this, "Success", "Medicine added successfully.");
            populateStockTable();
        } else {
            QMessageBox::critical(this, "Database Error", "Failed to add medicine.");
        }
    }
}

void MainWindow::onEditMedicineClicked()
{
    int row = m_stockTableWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a medicine to edit.");
        return;
    }

    QVariantList data;
    for(int i = 0; i < m_stockTableWidget->columnCount(); ++i) {
        data << m_stockTableWidget->item(row, i)->data(Qt::DisplayRole);
    }
    int medicineId = data[0].toInt();

    AddMedicineDialog dialog(data, this);
    if (dialog.exec() == QDialog::Accepted) {
        // --- THIS WAS THE MISSING PART ---
        // Declare and initialize the variables by getting data from the dialog
        QString name = dialog.name();
        QString batch = dialog.batchNumber();
        QString expiry = dialog.expiryDate();
        int qty = dialog.quantity();
        double price = dialog.price();
        // ------------------------------------

        if (m_dbManager->updateMedicine(medicineId, name, batch, expiry, qty, price)) {
            QMessageBox::information(this, "Success", "Medicine updated successfully.");
            populateStockTable();
        } else {
            QMessageBox::critical(this, "Database Error", "Failed to update medicine.");
        }
    }
}

void MainWindow::updateTotalAmount()
{
    double total = 0.0;
    for (int i = 0; i < m_cartListWidget->count(); ++i) {
        QListWidgetItem *item = m_cartListWidget->item(i);
        total += item->data(Qt::UserRole + 1).toDouble();
    }
    m_totalAmountLabel->setText(QString("Total: $ %1").arg(total, 0, 'f', 2));
}

void MainWindow::onStockTableDoubleClicked(int row, int column)
{
    qDebug() << "onStockTableDoubleClicked triggered for row:" << row;

    // --- Step 1: Get all necessary data from the selected row ---
    int medicineId = m_stockTableWidget->item(row, 0)->text().toInt();
    QString name = m_stockTableWidget->item(row, 1)->text();
    QString expiryDateString = m_stockTableWidget->item(row, 3)->text(); // Get the expiry date string
    int availableQty = m_stockTableWidget->item(row, 4)->text().toInt();
    double price = m_stockTableWidget->item(row, 5)->text().toDouble();

    // --- Step 2: Perform the critical expiry check FIRST ---
    QDate expiryDate = QDate::fromString(expiryDateString, "yyyy-MM-dd");
    if (expiryDate < QDate::currentDate()) {
        QMessageBox::critical(this, "Expired Medicine Alert",
                              QString("Cannot sell '%1'.\n\nReason: Medicine expired on %2.")
                                  .arg(name, expiryDate.toString("dd MMMM yyyy")));
        // Stop the function immediately. Do not proceed to sell.
        return;
    }

    // --- Step 3: Perform the existing "out of stock" check ---
    if (availableQty <= 0) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Out of Stock");
        msgBox.setText(QString("'%1' is out of stock.").arg(name));
        msgBox.setInformativeText("What would you like to do?");
        QPushButton *addStockButton = msgBox.addButton("Add Stock", QMessageBox::ActionRole);
        QPushButton *editInfoButton = msgBox.addButton("Edit Info", QMessageBox::ActionRole);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.exec();

        if (msgBox.clickedButton() == addStockButton) {
            onAddStockClicked(); // Reuse the context menu slot
        } else if (msgBox.clickedButton() == editInfoButton) {
            onEditMedicineClicked(); // Reuse the context menu slot
        }
        // Stop the function. Do not proceed to sell.
        return;
    }

    // --- Step 4: If all checks pass, proceed to add to cart ---
    bool ok;
    int qtyToSell = QInputDialog::getInt(this, "Select Quantity",
                                         QString("Enter quantity for %1:").arg(name), 1, 1, availableQty, 1, &ok);

    if (ok) {
        double subtotal = qtyToSell * price;
        QString cartText = QString("💊 %1x %2 @ $%3 = $%4")
                               .arg(qtyToSell).arg(name).arg(price, 0, 'f', 2).arg(subtotal, 0, 'f', 2);
        QListWidgetItem *cartItem = new QListWidgetItem(cartText, m_cartListWidget);
        cartItem->setData(Qt::UserRole, medicineId);
        cartItem->setData(Qt::UserRole + 1, subtotal);
        cartItem->setData(Qt::UserRole + 2, qtyToSell);
        updateTotalAmount();
    }
}
void MainWindow::onFinalizeSaleClicked()
{
    if (m_cartListWidget->count() == 0) {
        QMessageBox::warning(this, "Empty Cart", "The cart is empty.");
        return;
    }

    QList<QPair<int, int>> cartItems;
    double totalAmount = 0.0;
    for (int i = 0; i < m_cartListWidget->count(); ++i) {
        QListWidgetItem* item = m_cartListWidget->item(i);
        cartItems.append({item->data(Qt::UserRole).toInt(), item->data(Qt::UserRole + 2).toInt()});
        totalAmount += item->data(Qt::UserRole + 1).toDouble();
    }

    qint64 invoiceId = m_dbManager->createInvoice(totalAmount, cartItems);
    if (invoiceId != -1) {
        QMessageBox::information(this, "Success", QString("Sale finalized successfully!\nInvoice ID: %1").arg(invoiceId));
        m_cartListWidget->clear();
        updateTotalAmount();
        populateStockTable();
    } else {
        QMessageBox::critical(this, "Database Error", "Failed to finalize the sale.");
    }
}

void MainWindow::onSearchQueryChanged(const QString& text)
{
    for (int i = 0; i < m_stockTableWidget->rowCount(); ++i) {
        QTableWidgetItem *item = m_stockTableWidget->item(i, 1);
        if (item) {
            bool match = item->text().contains(text, Qt::CaseInsensitive);
            m_stockTableWidget->setRowHidden(i, !match);
        }
    }
}

void MainWindow::onSalesHistoryClicked()
{
    // Create an instance of our SalesHistoryDialog
    SalesHistoryDialog dialog(m_dbManager, this);

    // Show the dialog modally (it will block the main window until closed)
    dialog.exec();
}


// --- ADD THIS ENTIRE BLOCK OF MISSING FUNCTIONS TO THE END OF mainwindow.cpp ---

void MainWindow::showTableContextMenu(const QPoint &pos)
{
    // Ensure we have a valid item at the clicked position
    QTableWidgetItem* item = m_stockTableWidget->itemAt(pos);
    if (!item) return;

    // Map the local position to a global position for the menu
    QPoint globalPos = m_stockTableWidget->mapToGlobal(pos);

    QMenu contextMenu;
    // Apply a modern style to the context menu if you have one
    // contextMenu.setStyleSheet("...");

    // Create the actions
    QAction *editAction = contextMenu.addAction("✏️ Edit Medicine");
    QAction *deleteAction = contextMenu.addAction("🗑️ Delete Medicine");
    QAction *addStockAction = contextMenu.addAction("📦 Add Stock");

    // Execute the menu and get the chosen action
    QAction *selectedAction = contextMenu.exec(globalPos);

    // Call the appropriate slot based on the user's choice
    if (selectedAction == editAction) {
        onEditMedicineClicked();
    } else if (selectedAction == deleteAction) {
        onDeleteMedicineClicked();
    } else if (selectedAction == addStockAction) {
        onAddStockClicked();
    }
}

void MainWindow::onDeleteMedicineClicked()
{
    int row = m_stockTableWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a medicine to delete.");
        return;
    }

    QString medicineName = m_stockTableWidget->item(row, 1)->text();
    int medicineId = m_stockTableWidget->item(row, 0)->text().toInt();

    auto reply = QMessageBox::question(this, "Confirm Deletion",
                                       QString("Are you sure you want to permanently delete '%1'?").arg(medicineName),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (m_dbManager->deleteMedicine(medicineId)) { // You will need to add deleteMedicine to DatabaseManager
            QMessageBox::information(this, "Success", "Medicine deleted.");
            populateStockTable();
        } else {
            QMessageBox::critical(this, "Database Error", "Failed to delete medicine. It may be part of an existing invoice.");
        }
    }
}

void MainWindow::onAddStockClicked()
{
    int row = m_stockTableWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a medicine to add stock to.");
        return;
    }

    int medicineId = m_stockTableWidget->item(row, 0)->text().toInt();
    QString medicineName = m_stockTableWidget->item(row, 1)->text();

    bool ok;
    int qtyToAdd = QInputDialog::getInt(this, "Add Stock",
                                        QString("Enter quantity to add for '%1':").arg(medicineName),
                                        1, 1, 9999, 1, &ok);

    if (ok) {
        if (m_dbManager->addStock(medicineId, qtyToAdd)) {
            QMessageBox::information(this, "Success", "Stock updated successfully.");
            populateStockTable();
        }
    }
}

// NOTE: The onAskCopilotClicked() and onGeminiReplyFinished() functions
// should already be in your mainwindow.cpp file from a previous step.
// If they are missing, re-add them. Here they are again for completeness.

void MainWindow::onAskCopilotClicked()
{
    QString symptoms = m_symptomsLineEdit->text().trimmed();
    if (symptoms.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter symptoms before asking Copilot.");
        return;
    }

    QString apiKey = "GEMINI_KEY"; // Remember to replace this
    if (apiKey.startsWith("YOUR_")) {
        QMessageBox::critical(this, "API Key Error", "Please add your Gemini API key in mainwindow.cpp.");
        return;
    }

    QStringList stockList;
    for (int i = 0; i < m_stockTableWidget->rowCount(); ++i) {
        if (!m_stockTableWidget->isRowHidden(i) && m_stockTableWidget->item(i, 4)->text().toInt() > 0) {
            stockList.append(m_stockTableWidget->item(i, 1)->text());
        }
    }

    if (stockList.isEmpty()) {
        QMessageBox::warning(this, "No Stock", "No medicines are in stock to make a suggestion.");
        return;
    }

    QString stockString = "- " + stockList.join("\n- ");
    QString promptText = QString(
                             "You are PharmaCopilot. Suggest suitable over-the-counter medicines from the provided list based on symptoms. "
                             "Only suggest from the list. If none are suitable, respond with 'None'. List one medicine per line.\n\n"
                             "Symptoms: %1\n\n"
                             "Available Medicines:\n%2"
                             ).arg(symptoms, stockString);

    QJsonObject textPart{{"text", promptText}};
    QJsonArray partsArray{textPart};
    QJsonObject contentObject{{"parts", partsArray}};
    QJsonArray contentsArray{contentObject};
    QJsonObject jsonBody{{"contents", contentsArray}};

    QNetworkRequest request(QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + apiKey));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_askCopilotButton->setText("Thinking...");
    m_askCopilotButton->setEnabled(false);
    m_suggestionsListWidget->clear();
    m_networkManager->post(request, QJsonDocument(jsonBody).toJson());
}

void MainWindow::onGeminiReplyFinished(QNetworkReply *reply)
{
    m_askCopilotButton->setText("Ask Copilot");
    m_askCopilotButton->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "API Error:" << reply->errorString() << reply->readAll();
        QMessageBox::critical(this, "API Error", "Failed to get a response from the AI service.");
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    QString resultText;
    QJsonArray candidates = jsonDoc.object()["candidates"].toArray();
    if (!candidates.isEmpty()) {
        resultText = candidates[0].toObject()["content"].toObject()["parts"].toArray()[0].toObject()["text"].toString().trimmed();
    }

    if (resultText.isEmpty() || resultText.toLower() == "none") {
        m_suggestionsListWidget->addItem("No suitable medicine found.");
    } else {
        m_suggestionsListWidget->addItems(resultText.split('\n', Qt::SkipEmptyParts));
    }
    reply->deleteLater();
}


void MainWindow::onClearCopilotClicked()
{
    // Simply clear the contents of the suggestions list widget
    if (m_suggestionsListWidget) {
        m_suggestionsListWidget->clear();
    }
}
