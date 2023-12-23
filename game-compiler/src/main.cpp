#include <mengine/mengine.h>

#include <mrender/entry.h>
#include <mrender/input.h>

#include <imgui/imgui.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stbimage.h"

#include "compiler_fbx.h"


namespace 
{
	struct Vertex
	{
		float x;
		float y;
		float z;
		float u;
		float v;
	};

	Vertex cubeVertices[] =
	{
		{-1.0f, -1.0f, 1.0f, 0.0f, 0.0f}, 
		{ 1.0f, -1.0f, 1.0f, 1.0f, 0.0f}, 
		{ 1.0f,  1.0f, 1.0f, 1.0f, 1.0f}, 
		{-1.0f,  1.0f, 1.0f, 0.0f, 1.0f}, 
		{ 1.0f, -1.0f, -1.0f, 0.0f, 0.0f},
		{-1.0f, -1.0f, -1.0f, 1.0f, 0.0f},
		{-1.0f,  1.0f, -1.0f, 1.0f, 1.0f},
		{ 1.0f,  1.0f, -1.0f, 0.0f, 1.0f},
		{ 1.0f, -1.0f,  1.0f, 0.0f, 0.0f},
		{ 1.0f, -1.0f, -1.0f, 1.0f, 0.0f},
		{ 1.0f,  1.0f, -1.0f, 1.0f, 1.0f},
		{ 1.0f,  1.0f,  1.0f, 0.0f, 1.0f},
		{-1.0f, -1.0f, -1.0f, 0.0f, 0.0f},
		{-1.0f, -1.0f,  1.0f, 1.0f, 0.0f},
		{-1.0f,  1.0f,  1.0f, 1.0f, 1.0f},
		{-1.0f,  1.0f, -1.0f, 0.0f, 1.0f},
		{-1.0f,  1.0f,  1.0f, 0.0f, 0.0f},
		{ 1.0f,  1.0f,  1.0f, 1.0f, 0.0f},
		{ 1.0f,  1.0f, -1.0f, 1.0f, 1.0f},
		{-1.0f,  1.0f, -1.0f, 0.0f, 1.0f},
		{-1.0f, -1.0f, -1.0f, 0.0f, 0.0f},
		{ 1.0f, -1.0f, -1.0f, 1.0f, 0.0f},
		{ 1.0f, -1.0f,  1.0f, 1.0f, 1.0f},
		{-1.0f, -1.0f,  1.0f, 0.0f, 1.0f} 
	};

	const U16 cubeTriList[] =
	{
		0, 1, 2,
		2, 3, 0,
		4, 5, 6,
		6, 7, 4,
		8, 9, 10,
		10, 11, 8,
		12, 13, 14,
		14, 15, 12,
		16, 17, 18,
		18, 19, 16,
		20, 21, 22,
		22, 23, 20
	};

	// Game
	class GameCompiler : public mrender::AppI
	{
	public:
		GameCompiler(const char* _name, const char* _description)
			: mrender::AppI(_name, _description)
		{
			mrender::setWindowTitle(mrender::kDefaultWindowHandle, "Game Resource Compiler | Marcus Madland");
		}

		void init(I32 _argc, const char* const* _argv, U32 _width, U32 _height) override
		{
			// Engine
			mengine::Init mengineInit;
			mengineInit.graphicsApi = bgfx::RendererType::Direct3D11;
			mengineInit.resolution.width = _width;
			mengineInit.resolution.height = _height;
			mengine::init(mengineInit);

			{	// MODEL
				mengine::GeometryCreate data = createResourceFromFBX(
					"C:/Users/marcu/Dev/mengine-demo/game-compiler/resources/katana.fbx", 0,
					"meshes/katana.bin");
			}

			{	// CUBE
				bgfx::VertexLayout layout;
				layout.begin()
					.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
					.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
					.end();

				mengine::GeometryCreate data;
				data.vertices = cubeVertices;
				data.verticesSize = sizeof(Vertex) * 24;
				data.indices = cubeTriList;
				data.indicesSize = sizeof(U16) * 36;
				data.layout = layout;
				mengine::createResource(data, 
					"meshes/cube.bin");
			}

			{	// VERTEX SHADER
				mengine::ShaderCreate data;
				int argc = 0;
				const char* argv[16];
				argv[argc++] = "-f";
				argv[argc++] = "C:/Users/marcu/Dev/mengine-demo/game-compiler/resources/vs_cube.sc";
				argv[argc++] = "--varyingdef";
				argv[argc++] = "C:/Users/marcu/Dev/mengine-demo/game-compiler/resources/varying.def.sc";
				argv[argc++] = "--type";
				argv[argc++] = "v";
				argv[argc++] = "--platform";
				argv[argc++] = "windows";
				argv[argc++] = "--profile";
				argv[argc++] = "s_5_0";
				argv[argc++] = "--O";
				data.mem = bgfx::compileShader(argc, argv);
				mengine::createResource(data, "shaders/vs_cube.bin");
			}

			{	// FRAGMENT SHADER
				mengine::ShaderCreate data;
				int argc = 0;
				const char* argv[16];
				argv[argc++] = "-f";
				argv[argc++] = "C:/Users/marcu/Dev/mengine-demo/game-compiler/resources/fs_cube.sc";
				argv[argc++] = "--varyingdef";
				argv[argc++] = "C:/Users/marcu/Dev/mengine-demo/game-compiler/resources/varying.def.sc";
				argv[argc++] = "--type";
				argv[argc++] = "f";
				argv[argc++] = "--platform";
				argv[argc++] = "windows";
				argv[argc++] = "--profile";
				argv[argc++] = "s_5_0";
				argv[argc++] = "--O";
				data.mem = bgfx::compileShader(argc, argv);
				mengine::createResource(data, "shaders/fs_cube.bin");
			}

			stbi_set_flip_vertically_on_load(true);

			{	// TEXTURE
				mengine::TextureCreate data;
				int width, height, channels;
				unsigned char* mem = stbi_load("C:/Users/marcu/Dev/mengine-demo/game-compiler/resources/stone.jpg", &width, &height, &channels, STBI_rgb);
				data.width = width;
				data.height = height;
				data.hasMips = false;
				data.format = bgfx::TextureFormat::RGB8;
				data.flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE;
				data.mem = mem;
				data.memSize = width * height * channels;
				mengine::createResource(data, "textures/mc.bin");
			}

			{	// TEXTURE
				mengine::TextureCreate data;
				int width, height, channels;
				unsigned char* mem = stbi_load("C:/Users/marcu/Dev/mengine-demo/game-compiler/resources/katana.jpg", &width, &height, &channels, STBI_rgb);
				data.width = width;
				data.height = height;
				data.hasMips = false;
				data.format = bgfx::TextureFormat::RGB8;
				data.flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE;
				data.mem = mem;
				data.memSize = width * height * channels;
				mengine::createResource(data, "textures/katana.bin");
			}

			{	// MATERIAL
				mengine::ResourceHandle texture = mengine::loadTexture("textures/mc.bin");

				mengine::MaterialParameters parameters;
				parameters.begin()
					.addTexture("s_texture", texture)
					.end();

				mengine::MaterialCreate data;
				data.vertShaderPath = "shaders/vs_cube.bin";
				data.fragShaderPath = "shaders/fs_cube.bin";
				data.parameters = parameters;
				mengine::createResource(data, "materials/red.bin");
			}

			{	// MATERIAL
				mengine::ResourceHandle texture = mengine::loadTexture("textures/katana.bin");

				mengine::MaterialParameters parameters;
				parameters.begin()
					.addTexture("s_texture", texture)
					.end();

				mengine::MaterialCreate data;
				data.vertShaderPath = "shaders/vs_cube.bin";
				data.fragShaderPath = "shaders/fs_cube.bin";
				data.parameters = parameters;
				mengine::createResource(data, "materials/katana.bin");
			}

			mengine::packAssets("C:/Users/marcu/Dev/mengine-demo/game/build/bin/data/assets.pak");
		}

		int shutdown() override
		{
			mengine::shutdown();

			return 0;
		}

		bool update() override
		{
			if (mengine::update(BGFX_DEBUG_TEXT, BGFX_RESET_VSYNC))
			{
				bgfx::frame();

				return true;
			}
			
			return false;
		}
	};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	::GameCompiler
	, "Game Compiler"
	, "An example of a game resource compiler"
);