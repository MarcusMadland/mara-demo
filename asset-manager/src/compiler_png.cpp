#include "compiler_png.h"

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#include "mengine/mengine.h"
#include "mrender/bgfx.h"

namespace am {

void compileTextureFromPNG(const AssetDetails& _assetDetails)
{
    stbi_set_flip_vertically_on_load(true);
    int width, height, numChannels;
    U8* imageData = stbi_load(_assetDetails.sourceFilePath.getCPtr(), &width, &height, &numChannels, 3);
    int imageSize = (width * height) * numChannels;

    if (imageData) 
    {
        bx::DefaultAllocator allocator;
        mengine::TextureAsset texture = mengine::TextureAsset();
    	texture.m_memory = bgfx::makeRef(imageData, (U32)imageSize, &allocator);

        // Serialize
        bx::FilePath outputPath = _assetDetails.outputDirectoryPath;
        outputPath.join(_assetDetails.name.toUtf8().constData());
        outputPath.join(".bin", false); // @todo make custom type

    	if (!texture.serialize(outputPath))
        {
            bx::debugPrintf("Failed to serialize asset\n");
        }

        stbi_image_free(imageData);
    }
}

}
