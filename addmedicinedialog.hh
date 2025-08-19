#ifndef ADDMEDICINEDIALOG_H
#define ADDMEDICINEDIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations for UI elements
class QLineEdit;
class QDateEdit;
class QSpinBox;
class QDoubleSpinBox;

class AddMedicineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddMedicineDialog(QWidget *parent = nullptr);
    // **NEW**: Constructor for editing existing medicines
    explicit AddMedicineDialog(const QVariantList& medicineData, QWidget *parent = nullptr);

    // Public functions to get the data entered by the user
    QString name() const;
    QString batchNumber() const;
    QString expiryDate() const;
    int quantity() const;
    double price() const;

private:
    void setupDialogUI();

    QLineEdit *m_nameEdit;
    QLineEdit *m_batchNumberEdit;
    QDateEdit *m_expiryDateEdit;
    QSpinBox *m_quantitySpinBox;
    QDoubleSpinBox *m_priceSpinBox;
};

#endif // ADDMEDICINEDIALOG_H
