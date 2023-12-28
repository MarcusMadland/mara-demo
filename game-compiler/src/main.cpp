
// mengine
#include <mengine/mengine.h>
#include <mrender/entry.h>

// 3rd-party
#define STB_IMAGE_IMPLEMENTATION
#include "stbimage.h"
#include "ufbx.h"

#include <vector>

namespace 
{
	#define RESOURCE_LOCATION "C:/Users/marcu/Dev/mengine-demo/game-compiler/resources/"

	void importShader(const bx::FilePath& _shaderPath, const bx::FilePath& _varyingPath, 
		const bx::FilePath& _outVfp)
	{
		mengine::ShaderCreate data;
		int argc = 0;
		const char* argv[16];

		argv[argc++] = "-f";
		argv[argc++] = RESOURCE_LOCATION "vs_cube.sc";
		argv[argc++] = "--varyingdef";
		argv[argc++] = RESOURCE_LOCATION "varying.def.sc";
		argv[argc++] = "--type";
		argv[argc++] = "v";
		argv[argc++] = "--platform";
		argv[argc++] = "windows";
		argv[argc++] = "--profile";
		argv[argc++] = "s_5_0";
		argv[argc++] = "--O";
		data.mem = bgfx::compileShader(argc, argv);

		mengine::createResource(data,
			"shaders/vs_cube.bin");
	}

	void importScene(const bx::FilePath& _fbxPath, const bx::FilePath& _outVfp)
	{
		struct MeshVertex
		{
			float x;
			float y;
			float z;
			float u;
			float v;
			float nx;
			float ny;
			float nz;

			bool operator==(const MeshVertex& other) const
			{
				return x == other.x && y == other.y && z == other.z &&
					u == other.u && v == other.v &&
					nx == other.nx && ny == other.ny && nz == other.nz;
			}
		};

		// Load FBX
		ufbx_load_opts opts;
		ufbx_error err;
		ufbx_scene* scene = ufbx_load_file(_fbxPath.getCPtr(), &opts, &err);
		if (!scene)
		{
			BX_TRACE("Failed to load fbx file at %s", _fbxPath.getCPtr())
			return;
		}

		// Create a data structure to hold unique vertices and indices.
		std::vector<MeshVertex> uniqueVertices;
		std::vector<U16> indices;

		// Handle Mesh
		U16 meshCount = 0;
		for (size_t i = 0; i < scene->nodes.count; i++)
		{
			ufbx_node* node = scene->nodes.data[i];
			if (node->is_root) continue;

			// Handle Mesh
			if (node->mesh)
			{
				if (meshCount != _meshIndex)
				{
					meshCount++;
					continue;
				}
				meshCount++;

				// Process Faces
				for (U32 j = 0; j < node->mesh->faces.count; j++)
				{
					ufbx_face face = node->mesh->faces.data[j];

					// Triangulate the face
					std::vector<U32> triIndices;
					triIndices.resize(node->mesh->max_face_triangles * 3);
					U32 numTris = ufbx_triangulate_face(triIndices.data(), triIndices.size(), node->mesh, face);

					for (U32 k = 0; k < numTris; k++)
					{
						for (U32 l = 0; l < 3; l++)
						{
							U16 index = triIndices[k * 3 + l];

							MeshVertex vertex;
							vertex.x = (F32)node->mesh->vertex_position[index].x;
							vertex.y = (F32)node->mesh->vertex_position[index].y;
							vertex.z = (F32)node->mesh->vertex_position[index].z;

							if (node->mesh->vertex_uv.exists)
							{
								vertex.u = (F32)node->mesh->vertex_uv.values[node->mesh->vertex_uv.indices[index]].x;
								vertex.v = (F32)node->mesh->vertex_uv.values[node->mesh->vertex_uv.indices[index]].y;
							}

							if (node->mesh->vertex_normal.exists)
							{
								vertex.nx = (F32)node->mesh->vertex_normal.values[node->mesh->vertex_normal.indices[index]].x;
								vertex.ny = (F32)node->mesh->vertex_normal.values[node->mesh->vertex_normal.indices[index]].y;
								vertex.nz = (F32)node->mesh->vertex_normal.values[node->mesh->vertex_normal.indices[index]].z;
							}

							auto it = std::find(uniqueVertices.begin(), uniqueVertices.end(), vertex);
							if (it == uniqueVertices.end())
							{
								uniqueVertices.push_back(vertex);
								indices.push_back(static_cast<U16>(uniqueVertices.size() - 1));
							}
							else
							{
								indices.push_back(static_cast<U16>(it - uniqueVertices.begin()));
							}
						}
					}
				}

				// Create Asset
				bgfx::VertexLayout layout;
				layout.begin()
					.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
					.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
					.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
					.end();

				mengine::GeometryCreate geometry;
				geometry.vertices = uniqueVertices.data();
				geometry.verticesSize = uniqueVertices.size() * sizeof(MeshVertex);
				geometry.indices = indices.data();
				geometry.indicesSize = indices.size() * sizeof(U16);
				geometry.layout = layout;
				mengine::createResource(geometry, _outVfp);

				ufbx_free_scene(scene);

				return;
			}
			break;
		}

		ufbx_free_scene(scene);
		return;
	}

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
			// Init engine
			mengine::Init mengineInit;
			mengineInit.graphicsApi = bgfx::RendererType::Direct3D11;
			mengineInit.resolution.width = _width;
			mengineInit.resolution.height = _height;
			mengine::init(mengineInit);

			// Import shaders from sc
			importShader(RESOURCE_LOCATION "vs_cube.sc", RESOURCE_LOCATION "varying.def.sc",
				"shaders/vs_cube.bin");

			importShader(RESOURCE_LOCATION "fs_cube.sc", RESOURCE_LOCATION "varying.def.sc",
				"shaders/fs_cube.bin");

			// Import scene from fbx
			importScene(RESOURCE_LOCATION "scene/scene.fbx",
				"scenes/scene.bin");

			// Package cooked resources into one big file
			mengine::createPak("C:/Users/marcu/Dev/mengine-demo/game/build/bin/data/assets.pak");
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