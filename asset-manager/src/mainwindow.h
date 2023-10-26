#ifndef mainwindow_h
#define mainwindow_h

#include <QMainWindow>
#include <QScopedPointer>
#include <QTableWidget>
#include <QListWidget>
#include <QGridLayout>
#include <QProgressBar>
#include <mapp/filepath.h>

#include "asset_manager.h"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

private:
    void openImportDialog();
    void updateAssetList();
    void updateAssetDetails();

    void onEditButtonClicked();
    void onRemoveButtonClicked();

    void onCompileSelected();
    void onCompileMissing();
    void onCompileAll();

    void compile(const am::AssetDetails& _assetDetails);

private:
    QScopedPointer<Ui::MainWindow> ui;

    bx::FilePath m_settingsPath;

    QTableWidget* m_assetListWidget;
    QListWidget* m_assetDetailsWidget;
    QProgressBar* m_progressBar;

    am::AssetDetails m_selectedAssetDetails;
};

#endif
