#include "addmedicinedialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>

AddMedicineDialog::AddMedicineDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Add New Medicine");
    setModal(true); // Makes the dialog block the main window

    setupDialogUI();
}

// **NEW** Edit Mode Constructor
AddMedicineDialog::AddMedicineDialog(const QVariantList& medicineData, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Medicine");
    setModal(true);

    setupDialogUI(); // Build the UI first

    // Pre-fill the fields with existing data
    // Assumes data order: ID, Name, Batch, Expiry, Quantity, Price
    if (medicineData.count() >= 6) {
        m_nameEdit->setText(medicineData[1].toString());
        m_batchNumberEdit->setText(medicineData[2].toString());
        m_expiryDateEdit->setDate(QDate::fromString(medicineData[3].toString(), "yyyy-MM-dd"));
        m_quantitySpinBox->setValue(medicineData[4].toInt());
        m_priceSpinBox->setValue(medicineData[5].toDouble());
    }
}

void AddMedicineDialog::setupDialogUI()
{
    // Create layouts
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    // Create UI elements
    m_nameEdit = new QLineEdit(this);
    m_batchNumberEdit = new QLineEdit(this);
    m_expiryDateEdit = new QDateEdit(QDate::currentDate().addYears(1), this);
    m_expiryDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_expiryDateEdit->setCalendarPopup(true);
    m_quantitySpinBox = new QSpinBox(this);
    m_quantitySpinBox->setRange(0, 9999);
    m_priceSpinBox = new QDoubleSpinBox(this);
    m_priceSpinBox->setRange(0.0, 9999.99);
    m_priceSpinBox->setDecimals(2);
    m_priceSpinBox->setPrefix("$ ");

    // Add rows to the form layout
    formLayout->addRow("Name:", m_nameEdit);
    formLayout->addRow("Batch Number:", m_batchNumberEdit);
    formLayout->addRow("Expiry Date:", m_expiryDateEdit);
    formLayout->addRow("Quantity:", m_quantitySpinBox);
    formLayout->addRow("Price:", m_priceSpinBox);

    // Create standard OK/Cancel buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Add layouts to the main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
}

// ---- Data retrieval functions ----

QString AddMedicineDialog::name() const
{
    return m_nameEdit->text();
}

QString AddMedicineDialog::batchNumber() const
{
    return m_batchNumberEdit->text();
}

QString AddMedicineDialog::expiryDate() const
{
    return m_expiryDateEdit->date().toString("yyyy-MM-dd");
}

int AddMedicineDialog::quantity() const
{
    return m_quantitySpinBox->value();
}

double AddMedicineDialog::price() const
{
    return m_priceSpinBox->value();
}
