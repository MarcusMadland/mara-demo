#include "mainwindow.h"  
#include "ui_mainwindow.h"  

#include <QDockWidget>     
#include <QGridLayout>
#include <QTableWidget>    
#include <QTextEdit>
#include <QMenuBar>
#include <QProgressBar>
#include <QStatusBar>
#include <QListWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>

#include "asset_manager.h"
#include "importwindow.h"

#include "compiler_fbx.h"
#include "compiler_sc.h"
#include "compiler_png.h"

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent), ui(new Ui::MainWindow)
{
    // Create settings path @todo Make it in a folder in documents/ 
    m_settingsPath = bx::FilePath(bx::Dir::Executable);
    m_settingsPath = m_settingsPath.getPath();
    m_settingsPath.join("asset_manager.ini");

    // Load Settings
    am::loadSettings(m_settingsPath);

    // Setup UI
    ui->setupUi(this);

    // Set window title
    setWindowTitle("Asset Manager");

    // Create a menu bar
    QMenuBar* menuBar = this->menuBar();
    QMenu* menuFile = menuBar->addMenu("File");
    QMenu* menuCompile = menuBar->addMenu("Compile");
    QAction* actionImport = new QAction("Import", this);
    QAction* actionCompSel = new QAction("Compile Selected", this);
    QAction* actionCompMiss = new QAction("Compile Missing", this);
    QAction* actionRecompAll = new QAction("Recompile All", this);

    menuFile->addAction(actionImport);
    menuCompile->addAction(actionCompSel);
    menuCompile->addAction(actionCompMiss);
    menuCompile->addSeparator();
    menuCompile->addAction(actionRecompAll);

    // Create a toolbar
    QToolBar* toolbar = addToolBar("Main Toolbar");

    // Create a progressbar
    m_progressBar = new QProgressBar(this);

    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(1);
    m_progressBar->setValue(0);

    statusBar()->addPermanentWidget(m_progressBar);

    // Create a dock widget for the asset list
    QDockWidget* assetListDock = new QDockWidget("Asset List", this);
    m_assetListWidget = new QTableWidget(assetListDock);
    assetListDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    assetListDock->setWidget(m_assetListWidget);
    assetListDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::LeftDockWidgetArea, assetListDock);


    // Create a dock widget for asset details
    QDockWidget* assetDetailsDock = new QDockWidget("Asset Details", m_assetListWidget);
    m_assetDetailsWidget = new QListWidget();
    assetDetailsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    assetDetailsDock->setWidget(m_assetDetailsWidget);
    assetDetailsDock->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::RightDockWidgetArea, assetDetailsDock);

    // Set central widget
    setCentralWidget(assetListDock);

    // Update asset list
    updateAssetList();

    // Connects
    connect(actionImport, &QAction::triggered, this, &MainWindow::openImportDialog);
    connect(actionCompSel, &QAction::triggered, this, &MainWindow::onCompileSelected);
    connect(actionCompMiss, &QAction::triggered, this, &MainWindow::onCompileMissing);
    connect(actionRecompAll, &QAction::triggered, this, &MainWindow::onCompileAll);

    connect(m_assetListWidget, &QTableWidget::itemSelectionChanged, this, &MainWindow::updateAssetDetails);
}

MainWindow::~MainWindow()
{
}

void MainWindow::openImportDialog()
{
    ImportDialog importDialog = ImportDialog(this);
    importDialog.exec();
    am::saveSettings(m_settingsPath);

    updateAssetList();
}

void MainWindow::updateAssetList()
{
    am::updateAssetIsCompiled();

    m_assetListWidget->clear();

    m_assetListWidget->setRowCount(am::getAssetDetailsList().size());
    m_assetListWidget->setColumnCount(5);
    m_assetListWidget->setColumnWidth(0, 50);
    m_assetListWidget->setColumnWidth(1, 200);
    m_assetListWidget->setColumnWidth(2, 200);
    m_assetListWidget->setColumnWidth(3, 350);
    m_assetListWidget->setColumnWidth(4, 350);
    m_assetListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_assetListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList headersH;
    headersH << "ID" << "Name" << "Type" << "Source" << "Output";
    m_assetListWidget->setHorizontalHeaderLabels(headersH);
    m_assetListWidget->verticalHeader()->setVisible(false);
    m_assetListWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    for (U16 i = 0; i < am::getAssetDetailsList().size(); i++)
    {
        am::AssetDetails asset = am::getAssetDetailsList()[i];

        QTableWidgetItem* itemID = new QTableWidgetItem(QString::number(asset.id));
        QTableWidgetItem* itemSource = new QTableWidgetItem(asset.sourceFilePath.getCPtr());
        QTableWidgetItem* itemOutput = new QTableWidgetItem(asset.outputDirectoryPath.getCPtr());
        QTableWidgetItem* itemName = new QTableWidgetItem(asset.name);
        QTableWidgetItem* itemType = new QTableWidgetItem(asset.type);

        itemID->setFlags(itemID->flags() ^ Qt::ItemIsEditable);
        itemSource->setFlags(itemSource->flags() ^ Qt::ItemIsEditable);
        itemOutput->setFlags(itemOutput->flags() ^ Qt::ItemIsEditable);
        itemName->setFlags(itemName->flags() ^ Qt::ItemIsEditable);
        itemType->setFlags(itemType->flags() ^ Qt::ItemIsEditable);

        if (asset.compiled)
        {
            itemOutput->setForeground(QBrush(QColor(0, 0, 0, 255)));
        }
        else
        {
            itemOutput->setForeground(QBrush(QColor(255, 0, 0, 255)));
        }

        m_assetListWidget->setItem(i, 0, itemID);
        m_assetListWidget->setItem(i, 3, itemSource);
        m_assetListWidget->setItem(i, 4, itemOutput);
        m_assetListWidget->setItem(i, 1, itemName);
        m_assetListWidget->setItem(i, 2, itemType);
    }
}

void MainWindow::updateAssetDetails()
{
    QList<QTableWidgetItem*> selectedItems = m_assetListWidget->selectedItems();

    if (!selectedItems.isEmpty())
    {
        int row = selectedItems.first()->row();

        QTableWidgetItem* itemID = m_assetListWidget->item(row, 0);
        QTableWidgetItem* itemName = m_assetListWidget->item(row, 1);
        QTableWidgetItem* itemType = m_assetListWidget->item(row, 2);
        QTableWidgetItem* itemSource = m_assetListWidget->item(row, 3);
        QTableWidgetItem* itemOutput = m_assetListWidget->item(row, 4);

        m_selectedAssetDetails.id = itemID->text().toInt();
        m_selectedAssetDetails.name = itemName->text();
        m_selectedAssetDetails.type = itemType->text();
        m_selectedAssetDetails.sourceFilePath = bx::FilePath(itemSource->text().toUtf8().constData());
        m_selectedAssetDetails.outputDirectoryPath = bx::FilePath(itemOutput->text().toUtf8().constData());

        m_assetDetailsWidget->clear();

        QWidget* customItemWidget = new QWidget;
        QGridLayout* layout = new QGridLayout(customItemWidget);
        layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

        QLabel* labelName = new QLabel("Name: " + m_selectedAssetDetails.name);
        QLabel* labelType = new QLabel("Type: " + m_selectedAssetDetails.type);
        QLabel* labelSource = new QLabel("Source: " + QString(m_selectedAssetDetails.sourceFilePath.getCPtr()));
        QLabel* labelOutput = new QLabel("Output: " + QString(m_selectedAssetDetails.outputDirectoryPath.getCPtr()));

        QPushButton* buttonEdit = new QPushButton("Edit");
        buttonEdit->setMaximumWidth(100);
        QPushButton* buttonRemove = new QPushButton("Remove");
        buttonRemove->setMaximumWidth(100);

        layout->addWidget(labelName, 0, 0);
        layout->addWidget(labelType, 1, 0);
        layout->addWidget(labelSource, 2, 0);
        layout->addWidget(labelOutput, 3, 0);
        layout->addWidget(buttonEdit, 4, 0);
        layout->addWidget(buttonRemove, 5, 0);

        customItemWidget->setLayout(layout);

        QListWidgetItem* item = new QListWidgetItem(m_assetDetailsWidget);
        item->setSizeHint(customItemWidget->sizeHint());
        item->setFlags(item->flags() | Qt::ItemIsEnabled);

        m_assetDetailsWidget->setItemWidget(item, customItemWidget);

        connect(buttonEdit, &QPushButton::clicked, this, &MainWindow::onEditButtonClicked);
        connect(buttonRemove, &QPushButton::clicked, this, &MainWindow::onRemoveButtonClicked);
    }
}

void MainWindow::onEditButtonClicked()
{
    ImportDialog importDialog = ImportDialog(this, m_selectedAssetDetails, true);
    importDialog.exec();
    am::saveSettings(m_settingsPath);

    updateAssetList();
}

void MainWindow::onRemoveButtonClicked()
{
    am::removeAsset(m_selectedAssetDetails);
    am::updateAssetID();

    m_assetDetailsWidget->clear();
    updateAssetList();
    updateAssetDetails();
    am::saveSettings(m_settingsPath);
}

void MainWindow::onCompileSelected()
{
    QList<QTableWidgetItem*> selectedItems = m_assetListWidget->selectedItems();
    /* @todo Fix this
    for (U16 i = 0; i < selectedItems.size(); i++)
    {
        QTableWidgetItem* itemID = m_assetListWidget->item(i, 0);
        QTableWidgetItem* itemName = m_assetListWidget->item(i, 1);
        QTableWidgetItem* itemType = m_assetListWidget->item(i, 2);
        QTableWidgetItem* itemSource = m_assetListWidget->item(i, 3);
        QTableWidgetItem* itemOutput = m_assetListWidget->item(i, 4);

        am::AssetDetails assetDetails;
        assetDetails.id = itemID->text().toInt();
        assetDetails.name = itemName->text();
        assetDetails.type = itemType->text();
        assetDetails.sourceFilePath = bx::FilePath(itemSource->text().toUtf8().constData());
        assetDetails.outputDirectoryPath = bx::FilePath(itemOutput->text().toUtf8().constData());

        compile(assetDetails);
    }*/
}

void MainWindow::onCompileMissing()
{
    m_progressBar->setMaximum((U16)am::getAssetDetailsList().size());
    for (U16 i = 0; i < am::getAssetDetailsList().size(); i++)
    {
        m_progressBar->setValue(i);
        if (!am::getAssetDetailsList()[i].compiled)
        {
            compile(am::getAssetDetailsList()[i]);
        }
    }
    m_progressBar->setValue((U16)am::getAssetDetailsList().size());
}

void MainWindow::onCompileAll()
{
	m_progressBar->setMaximum((U16)am::getAssetDetailsList().size());
    for (U16 i = 0; i < am::getAssetDetailsList().size(); i++)
    {
        m_progressBar->setValue(i);
        compile(am::getAssetDetailsList()[i]);
    }
    m_progressBar->setValue((U16)am::getAssetDetailsList().size());
}

void MainWindow::compile(const am::AssetDetails& _assetDetails)
{
    bx::debugPrintf("Compiling asset with name: %s\n", _assetDetails.name.toUtf8().constData());

    if (strcmp(_assetDetails.sourceFilePath.getExt().getPtr(), ".fbx") == 0)
    {
        if (strcmp(_assetDetails.type.toUtf8().constData(), "Geometry") == 0)
        {
            am::compileGeometryFromFBX(_assetDetails, 0); // @todo make index a geometry import setting
        }
    }

    if (strcmp(_assetDetails.sourceFilePath.getExt().getPtr(), ".sc") == 0)
    {
	    if (strcmp(_assetDetails.type.toUtf8().constData(), "Vertex Shader") == 0)
	    {
	        am::compileVertexShaderFromSC(_assetDetails);
	    }

	    else if (strcmp(_assetDetails.type.toUtf8().constData(), "Fragment Shader") == 0)
	    {
	        am::compileFragmentShaderFromSC(_assetDetails);
	    }
    }

    if (strcmp(_assetDetails.sourceFilePath.getExt().getPtr(), ".png") == 0)
    {
        if (strcmp(_assetDetails.type.toUtf8().constData(), "Texture") == 0)
        {
            am::compileTextureFromPNG(_assetDetails);
        }
    }

    updateAssetList();
}
