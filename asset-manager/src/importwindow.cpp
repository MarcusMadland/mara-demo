#include "importwindow.h"

#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>

ImportDialog::ImportDialog(QWidget* _parent, const am::AssetDetails& _default, bool _reimport)
    : QDialog(_parent)
	, m_defaultAsset(_default)
	, m_reimport(_reimport)
{
    QGridLayout* layout = new QGridLayout(this);

    QLabel* labelName = new QLabel(this);
    labelName->setText("Asset name: ");

    QLabel* label1 = new QLabel(this);
    label1->setText("Source file: ");

    QLabel* label2 = new QLabel(this);
    label2->setText("Output directory: ");
    

    m_dirInput1 = new QLineEdit(this);
    m_dirInput1->setText(QString(m_defaultAsset.sourceFilePath.getCPtr()));
	m_dirInput2 = new QLineEdit(this);
    m_dirInput2->setText(QString(m_defaultAsset.outputDirectoryPath.getCPtr()));
    m_lineName = new QLineEdit(this);
    m_lineName->setText(m_defaultAsset.name);

    QPushButton* browseButton1 = new QPushButton("Browse", this);
    QPushButton* browseButton2 = new QPushButton("Browse", this);
    QPushButton* importButton = new QPushButton("Import", this);

    m_assetTypeComboBox = new QComboBox(this);
    m_assetTypeComboBox->addItem("Geometry");
    m_assetTypeComboBox->addItem("Vertex Shader");
    m_assetTypeComboBox->addItem("Fragment Shader");
    m_assetTypeComboBox->addItem("Texture");
    m_assetTypeComboBox->setCurrentText(m_defaultAsset.type);

    layout->addWidget(labelName, 0, 0);
    layout->addWidget(m_lineName, 0, 1, 1, 2);

    layout->addWidget(label1, 1, 0);
    layout->addWidget(m_dirInput1, 1, 1);
    layout->addWidget(browseButton1, 1, 2);

    layout->addWidget(label2, 2, 0);
    layout->addWidget(m_dirInput2, 2, 1); 
    layout->addWidget(browseButton2, 2, 2);

    layout->addWidget(m_assetTypeComboBox, 3, 0, 1, 3);

    layout->addWidget(importButton, 4, 0, 1, 1);

    setLayout(layout);

    if (false)
    {
        // Geometry Specific settings
    }

    connect(browseButton1, &QPushButton::clicked, this, &ImportDialog::browseDirectory1);
    connect(browseButton2, &QPushButton::clicked, this, &ImportDialog::browseDirectory2);
    connect(importButton, &QPushButton::clicked, this, &ImportDialog::importClicked);
}

void ImportDialog::browseDirectory1()
{
    QString filePathString = QFileDialog::getOpenFileName(this, "Select Source File", QDir::homePath());
    if (!filePathString.isEmpty())
    {
        m_dirInput1->setText(filePathString);

        const bx::FilePath filePath = filePathString.toUtf8().constData();
        const bx::FilePath fileName = filePath.getBaseName();
        m_lineName->setText(fileName.getCPtr());
    }
}

void ImportDialog::browseDirectory2()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, "Select Output Directory", QDir::homePath());
    if (!dirPath.isEmpty())
    {
        m_dirInput2->setText(dirPath);
    }
}

void ImportDialog::importClicked()
{
    am::AssetDetails asset = m_defaultAsset;
    asset.name = m_lineName->text();
    asset.type = m_assetTypeComboBox->currentText();
    asset.sourceFilePath = bx::FilePath(m_dirInput1->text().toUtf8().constData());
    asset.outputDirectoryPath = bx::FilePath(m_dirInput2->text().toUtf8().constData());

    if (m_reimport)
    {
        am::setAsset(m_defaultAsset.id, asset);
    }
    else
    {
        am::importAsset(asset);
    }

    accept();
}