#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "databasemanager.h"
#include "modernwidgets.h" // Include your new custom widgets
#include <QNetworkAccessManager>
#include <QNetworkReply>

// Forward declarations for standard Qt widgets
class QTableWidget;
class QListWidget;
class QLabel;
class QLineEdit;
class QFrame;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddMedicineClicked();
    void onEditMedicineClicked();
    void onDeleteMedicineClicked();
    void onAddStockClicked();
    void onFinalizeSaleClicked();
    void onClearCartClicked();
    void onSalesHistoryClicked();
    void onStockTableDoubleClicked(int row, int column);
    void onSearchQueryChanged(const QString& text);
    void showTableContextMenu(const QPoint &pos);
    void onAskCopilotClicked();
    void onGeminiReplyFinished(QNetworkReply *reply);
    void onClearCopilotClicked();

private:
    void populateStockTable();
    void updateTotalAmount();

    // Helper methods for modern UI
    QString getModernStyleSheet();
    void setupModernUI();
    QFrame* createModernFrame();
    void setupModernTable();
    void updateStockStats();

private:
    DatabaseManager *m_dbManager;

    // --- Core UI Components ---
    QTableWidget *m_stockTableWidget;
    QLineEdit *m_searchLineEdit;
    QListWidget *m_cartListWidget;
    QLabel *m_totalAmountLabel;

    // --- Modern Stats Cards ---
    StatsCard *m_totalStatsCard;
    StatsCard *m_lowStockCard;
    StatsCard *m_expiringCard;

    // --- PharmaCopilot UI ---
    QLineEdit *m_symptomsLineEdit;
    ModernButton *m_askCopilotButton;
    QListWidget *m_suggestionsListWidget;

    // --- Networking ---
    QNetworkAccessManager *m_networkManager;
};
#endif // MAINWINDOW_H
