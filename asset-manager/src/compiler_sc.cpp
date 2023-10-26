#include "compiler_sc.h"

#include "mapp/file.h"
#include "mapp/process.h"
#include <mapp/os.h>

#include <cstdlib> // for system function
#include <string>

namespace am {

void compileShaderFromSC(const char* _args)
{
    if (system(_args) != 0)
    {
        return;
    }
}

void compileVertexShaderFromSC(const AssetDetails& _assetDetails)
{
    bx::FilePath outputPath = _assetDetails.outputDirectoryPath;
    outputPath.join(_assetDetails.name.toUtf8().constData());
    outputPath.join(".bin", false); // @todo make custom type

    const int maxArgsLen = 256;
    char arguments[maxArgsLen];
    snprintf(arguments, maxArgsLen, "shaderc.bat -f %s -o %s --platform windows --profile 130 --type vertex --verbose -i ./", 
        _assetDetails.sourceFilePath.getCPtr(), outputPath.getCPtr());

    bx::makeAll(_assetDetails.outputDirectoryPath, bx::ErrorAssert{});
	compileShaderFromSC(arguments);
}
void compileFragmentShaderFromSC(const AssetDetails& _assetDetails)
{
    bx::FilePath outputPath = _assetDetails.outputDirectoryPath;
    outputPath.join(_assetDetails.name.toUtf8().constData());
    outputPath.join(".bin", false); // @todo make custom type

    const int maxArgsLen = 256;
    char arguments[maxArgsLen];
    snprintf(arguments, maxArgsLen, "shaderc.bat -f %s -o %s --platform windows --profile 130 --type fragment --verbose -i ./", 
        _assetDetails.sourceFilePath.getCPtr(), outputPath.getCPtr());

    bx::makeAll(_assetDetails.outputDirectoryPath, bx::ErrorAssert{});
	compileShaderFromSC(arguments);
}

}
