#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <QDialog>
#include <QLineEdit>

#include "asset_manager.h"

class ImportDialog : public QDialog
{
    Q_OBJECT

public:
    ImportDialog(QWidget* _parent = nullptr, const am::AssetDetails& _default = am::AssetDetails(), bool _reimport = false);

private slots:
    void browseDirectory1();
    void browseDirectory2();
    void importClicked();

private:
    QComboBox* m_assetTypeComboBox;
    QLineEdit* m_lineName;
    QLineEdit* m_dirInput1;
    QLineEdit* m_dirInput2;
    am::AssetDetails m_defaultAsset;
    bool m_reimport;
};

#endif // IMPORTDIALOG_H