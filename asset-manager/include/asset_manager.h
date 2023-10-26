#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <qstring.h>
#include <vector>

#include "mapp/types.h"
#include "mapp/filepath.h"

namespace am {

struct AssetDetails
{
	AssetDetails()
		: id(-1)
		, compiled(false)
		, name("Unnamed")
		, type("")
		, sourceFilePath(".")
		, outputDirectoryPath(".")
	{}

	U16 id; // Don't set this
	bool compiled; // Don't set this

	QString name;
	QString type;
	bx::FilePath sourceFilePath;
	bx::FilePath outputDirectoryPath;
};

void setAsset(U16 _id, const AssetDetails& _assetDetails);
bool importAsset(const AssetDetails& _assetDetails);
void removeAsset(const AssetDetails& _assetDetails);
void updateAssetIsCompiled();
void updateAssetID();
std::vector<AssetDetails> getAssetDetailsList();
void saveSettings(const bx::FilePath& _filePath);
void loadSettings(const bx::FilePath& _filePath);

}

#endif // ASSETMANAGER_H