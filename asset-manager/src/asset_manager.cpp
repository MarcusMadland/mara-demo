#include  "asset_manager.h"

#include <mapp/file.h>
#include <mapp/readerwriter.h>
#include <mapp/settings.h>

namespace am {

std::vector<AssetDetails> g_assetDetailsList;

void setAsset(U16 _id, const AssetDetails& _assetDetails)
{
	g_assetDetailsList[_id] = _assetDetails;
}

bool importAsset(const AssetDetails& _assetDetails)
{
	AssetDetails asset = _assetDetails;
	g_assetDetailsList.push_back(asset);

	updateAssetID();
	updateAssetIsCompiled();

	return true;
}

void removeAsset(const AssetDetails& _assetDetails)
{
	if (_assetDetails.id == 65535)
		return;

	if (g_assetDetailsList.size() <= 1)
	{
		g_assetDetailsList.erase(g_assetDetailsList.end() - 1);
	}
	else
	{
		g_assetDetailsList.erase(g_assetDetailsList.begin() + _assetDetails.id);
	}
}

void updateAssetIsCompiled()
{
	for (U16 i = 0; i < g_assetDetailsList.size(); i++)
	{
		bx::FilePath filePath = g_assetDetailsList[i].outputDirectoryPath;
		filePath.join(g_assetDetailsList[i].name.toUtf8().constData());
		filePath.join(".bin", false); // @todo custom extension

		bx::FileReader reader;
		if (bx::open(&reader, filePath))
		{
			g_assetDetailsList[i].compiled = true;
			bx::close(&reader);
		}
		else
		{
			g_assetDetailsList[i].compiled = false;
		}
	}
}

void updateAssetID()
{
	for (U16 i = 0; i < g_assetDetailsList.size(); i++)
	{
		g_assetDetailsList[i].id = i;
	}
}

std::vector<AssetDetails> getAssetDetailsList()
{
	return g_assetDetailsList;
}

void saveSettings(const bx::FilePath& _filePath)
{
	bx::DefaultAllocator allocator;
	bx::Settings settings(&allocator);

	char bufferAssetCount[256];
	bx::toString(bufferAssetCount, 256, g_assetDetailsList.size());
	settings.set("asset_count", bufferAssetCount);

	for (size_t i = 0; i < g_assetDetailsList.size(); ++i)
	{
		const AssetDetails& asset = g_assetDetailsList[i];
		char buffer[256];

		bx::toString(buffer, 10, asset.id);
		strcat(buffer, "_name");
		settings.set(buffer, asset.name.toUtf8().constData());

		bx::toString(buffer, 10, asset.id);
		strcat(buffer, "_type");
		settings.set(buffer, asset.type.toUtf8().constData());

		bx::toString(buffer, 10, asset.id);
		strcat(buffer, "_source");
		settings.set(buffer, asset.sourceFilePath.getCPtr());

		bx::toString(buffer, 10, asset.id);
		strcat(buffer, "_output");
		settings.set(buffer, asset.outputDirectoryPath.getCPtr());
	}

	bx::FileWriter writer;
	if (bx::open(&writer, _filePath));
	{
		bx::write(&writer, settings, bx::ErrorAssert{});
		bx::close(&writer);
	}
}

void loadSettings(const bx::FilePath& _filePath)
{
	// Create settings
	bx::DefaultAllocator allocator;
	bx::Settings settings = bx::Settings(&allocator);
	bx::FileReader reader;
	if (bx::open(&reader, _filePath))
	{
		bx::read(&reader, settings, bx::ErrorAssert{});
		bx::close(&reader);

		char* endPtr;
		U32 numAssets = strtoul(settings.get("asset_count").getPtr(), &endPtr, 10);
		for (U32 i = 0; i < numAssets; i++)
		{
			char iStr[32];
			bx::toString(iStr, 32, i);

			char baseName[64] = "";
			char baseType[64] = "";
			char baseSource[64] = "";
			char baseOutput[64] = "";

			strcpy(baseName, iStr);
			strcpy(baseType, iStr);
			strcpy(baseSource, iStr);
			strcpy(baseOutput, iStr);

			strcat(baseName, "_name");
			strcat(baseType, "_type");
			strcat(baseSource, "_source");
			strcat(baseOutput, "_output");

			AssetDetails asset;
			asset.name = QString(settings.get(baseName).getPtr());
			asset.type = QString(settings.get(baseType).getPtr());
			asset.sourceFilePath = bx::FilePath(settings.get(baseSource).getPtr());
			asset.outputDirectoryPath = bx::FilePath(settings.get(baseOutput).getPtr());

			importAsset(asset);
		}
	}
}

}
