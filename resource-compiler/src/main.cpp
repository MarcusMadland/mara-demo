
// mara
#include <mara/mara.h>
#include <graphics/entry.h>
#include <base/hash.h>

// 3rd-party
#define STB_IMAGE_IMPLEMENTATION
#include "stbimage.h"
#include "ufbx.h"

// std
#include <vector>
#include <string>

namespace 
{
	#define RESOURCE_LOCATION "C:/Users/marcu/Dev/mara-demo/resource-compiler/resources/"

	mara::ResourceHandle importShader(const base::FilePath& _shaderPath, const base::FilePath& _varyingPath, graphics::ShaderType::Enum _type,
		const base::FilePath& _outVfp)
	{
		mara::ShaderCreate data;
		int argc = 0;
		const char* argv[11];

		const char* type = "";
		if (_type == graphics::ShaderType::Vertex)   type = "v";
		if (_type == graphics::ShaderType::Fragment) type = "f";
		if (_type == graphics::ShaderType::Compute)  type = "c";

		argv[argc++] = "-f";
		argv[argc++] = _shaderPath.getCPtr();
		argv[argc++] = "--varyingdef";
		argv[argc++] = _varyingPath.getCPtr();
		argv[argc++] = "--type";
		argv[argc++] = type;
		argv[argc++] = "--platform";
		argv[argc++] = "windows";
		argv[argc++] = "--profile";
		argv[argc++] = "s_5_0";
		argv[argc++] = "--O";
		data.mem = graphics::compileShader(argc, argv);

		if (data.mem)
		{
			BASE_TRACE("Success: Loading shader at %s", _shaderPath.getCPtr())
			return mara::createResource(data, _outVfp);
		}
		else
		{
			BASE_TRACE("Failed: Loading shader at %s", _shaderPath.getCPtr())
			return MARA_INVALID_HANDLE;
		}
		
	}

	mara::ResourceHandle importTexture(const base::FilePath& _filePath,
		const base::FilePath& _outVfp)
	{
		// Flip image parsing
		stbi_set_flip_vertically_on_load(true);

		// Load image using stbi
		int texWidth, texHeight, texChannels;
		unsigned char* texData = stbi_load(_filePath.getCPtr(), &texWidth, &texHeight, &texChannels, STBI_rgb);

		// If image is valid, create engine texture resource
		if (NULL != texData)
		{
			graphics::TextureFormat::Enum texFormat = graphics::TextureFormat::RGB8;
			U64 texFlags = GRAPHICS_TEXTURE_NONE | GRAPHICS_SAMPLER_NONE;
			bool texMips = false;

			mara::TextureCreate texture;
			texture.width = texWidth;
			texture.height = texHeight;
			texture.flags = texFlags;
			texture.hasMips = texMips;
			texture.format = texFormat;
			texture.mem = texData;
			texture.memSize = texWidth * texHeight * texChannels;

			BASE_TRACE("Success: Loading texture at %s", _filePath.getCPtr())
			return mara::createResource(texture, _outVfp);
		}
		else
		{
			BASE_TRACE("Failed: Loading texture at %s", _filePath.getCPtr())
			return MARA_INVALID_HANDLE;
		}
	}

	mara::ResourceHandle importScene(const base::FilePath& _fbxPath, 
		const base::FilePath& _outVfp)
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
		ufbx_load_opts opts = {};
		ufbx_error err;
		ufbx_scene* scene = ufbx_load_file(_fbxPath.getCPtr(), &opts, &err);
		if (!scene)
		{
			BASE_TRACE("Failed to load fbx file at %s", _fbxPath.getCPtr())
			return MARA_INVALID_HANDLE;
		}
		
		std::vector<std::string> meshes;

		// Load Scene
		mara::ResourceHandle resource = MARA_INVALID_HANDLE;

		U32 meshId = 0;
		for (size_t i = 0; i < scene->nodes.count; i++)
		{
			// Create a data structure to hold unique vertices and indices.
			std::vector<MeshVertex>* uniqueVertices = new std::vector<MeshVertex>();
			std::vector<U16>* indices = new std::vector<U16>();

			// Get node
			ufbx_node* node = scene->nodes.data[i];
			if (node->is_root) continue;

			// Handle Mesh
			if (node->mesh)
			{
				const size_t maxExpectedVertices = scene->nodes.count * node->mesh->max_face_triangles * 3;
				uniqueVertices->reserve(maxExpectedVertices);
				indices->reserve(maxExpectedVertices);

				// Load geometry
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

							auto it = std::find(uniqueVertices->begin(), uniqueVertices->end(), vertex);
							if (it == uniqueVertices->end())
							{
								uniqueVertices->push_back(vertex);
								indices->push_back(static_cast<U16>(uniqueVertices->size() - 1));
							}
							else
							{
								indices->push_back(static_cast<U16>(it - uniqueVertices->begin()));
							}
						}
					}
				}

				// Load material
				mara::MaterialParameters parameters;

				ufbx_material* mat = node->materials[0];
				for (U32 j = 0; j < mat->textures.count; j++)
				{
					// Load material textures
					ufbx_string parameter = mat->textures[j].material_prop;
					const char* parameterMara = NULL;
					if (base::strCmp(parameter.data, "Maya|baseColor") == 0)
					{
						parameterMara = "s_diffuse";
					}
					else
					{
						continue;
					}
						
					ufbx_string pathToTexture = mat->textures[j].texture->absolute_filename;
					ufbx_string virtualPathToTexture = mat->textures[j].texture->relative_filename; // @todo Find more unique path

					base::FilePath texturePath = "textures";
					texturePath.join(base::FilePath(mat->textures[j].texture->relative_filename.data).getBaseName());
					texturePath.join(".bin", false);

					mara::ResourceHandle texture = importTexture(pathToTexture.data, texturePath);
					parameters.addTexture(parameterMara, texture, 0);
				}
				for (U32 j = 0; j < mat->props.props.count; j++)
				{
					// Load material properties
					ufbx_prop prop = mat->props.props[j];
					ufbx_string parameter = prop.name;
					const char* parameterMara = NULL;
					if (base::strCmp(parameter.data, "Maya|baseColor") == 0)
					{
						parameterMara = "u_diffuse";
					}
					else
					{
						continue;
					}

					ufbx_vec3 colorFbx = prop.value_vec3;
					F32 color[4] = { (F32)colorFbx.x, (F32)colorFbx.y, (F32)colorFbx.z, 1.0f };
					parameters.addVec4(parameterMara, color);
				}

				// Create Geometry Resource
				base::FilePath geometryPath = base::FilePath("geometry");
				{
					graphics::VertexLayout layout;
					layout.begin()
						.add(graphics::Attrib::Position, 3, graphics::AttribType::Float)
						.add(graphics::Attrib::TexCoord0, 2, graphics::AttribType::Float)
						.add(graphics::Attrib::Normal, 3, graphics::AttribType::Float)
						.end();

					mara::GeometryCreate geometry;
					geometry.vertices = uniqueVertices->data();
					geometry.verticesSize = uniqueVertices->size() * sizeof(MeshVertex);
					geometry.indices = indices->data();
					geometry.indicesSize = indices->size() * sizeof(U16);
					geometry.layout = layout;

					{
						U32 hash = base::hash<base::HashMurmur2A>(geometry.vertices, geometry.verticesSize);
						std::string hashAsString = std::to_string(hash);
						geometryPath.join(hashAsString.c_str());
					}
					geometryPath.join(".bin", false);
					mara::createResource(geometry, geometryPath);
				}

				// Create Material Resource
				base::FilePath materialPath = base::FilePath("material");
				{
					mara::MaterialCreate material;
					material.vertShaderPath = "shaders/vs_cube.bin";
					material.fragShaderPath = "shaders/fs_cube.bin";
					material.parameters = parameters;

					materialPath.join(mat->name.data);
					materialPath.join(".bin", false);
					mara::createResource(material, materialPath);
				}

				// Create Mesh Resource
				base::FilePath meshPath = base::FilePath("meshes");
				{
					mara::MeshCreate mesh;
					mesh.geometryPath = geometryPath;
					mesh.materialPath = materialPath;

					ufbx_matrix mtx = node->node_to_world;
					ufbx_vec3 col0 = mtx.cols[0];
					ufbx_vec3 col1 = mtx.cols[1];
					ufbx_vec3 col2 = mtx.cols[2];
					ufbx_vec3 col3 = mtx.cols[3];
					mesh.m_transform[0] = col0.x;
					mesh.m_transform[1] = col0.y;
					mesh.m_transform[2] = -col0.z;
					mesh.m_transform[3] = 0.0f;
					mesh.m_transform[4] = col1.x;
					mesh.m_transform[5] = col1.y;
					mesh.m_transform[6] = -col1.z;
					mesh.m_transform[7] = 0.0f;
					mesh.m_transform[8] = col2.x;
					mesh.m_transform[9] = col2.y;
					mesh.m_transform[10] = -col2.z;
					mesh.m_transform[11] = 0.0f;
					mesh.m_transform[12] = col3.x;
					mesh.m_transform[13] = col3.y;
					mesh.m_transform[14] = -col3.z;
					mesh.m_transform[15] = 1.0f;

					meshPath.join(node->name.data);
					meshPath.join(".bin", false);
					mara::createResource(mesh, meshPath);
				}

				meshes.push_back(meshPath.getCPtr());
				meshId++;
			}
		}

		// Create scene prefab
		{
			mara::PrefabCreate prefab;

			prefab.m_numMeshes = meshes.size();
			for (U32 i = 0; i < meshes.size(); i++)
			{
				prefab.meshPaths[i] = meshes[i].c_str();
			}

			resource = mara::createResource(prefab, _outVfp);
		}

		ufbx_free_scene(scene);
		return resource;
	}

	class GameCompiler : public entry::AppI
	{
	public:
		GameCompiler(const char* _name, const char* _description)
			: entry::AppI(_name, _description)
		{
			entry::setWindowTitle(entry::kDefaultWindowHandle, "Mara | Resource Compiler");
		}

		void init(I32 _argc, const char* const* _argv, U32 _width, U32 _height) override
		{
			// Init engine
			mara::Init maraInit;
			maraInit.graphicsApi = graphics::RendererType::Direct3D11;
			maraInit.resolution.width = _width;
			maraInit.resolution.height = _height;
			if (mara::init(maraInit))
			{
				graphics::setViewClear(0, GRAPHICS_CLEAR_COLOR | GRAPHICS_CLEAR_DEPTH, 0xFF00FFFF, 1.0f, 0);
			}

			// Import shaders from sc
			importShader(RESOURCE_LOCATION "vs_cube.sc", RESOURCE_LOCATION "varying.def.sc", graphics::ShaderType::Vertex,
				"shaders/vs_cube.bin");

			importShader(RESOURCE_LOCATION "fs_cube.sc", RESOURCE_LOCATION "varying.def.sc", graphics::ShaderType::Fragment,
				"shaders/fs_cube.bin");

			// Import character from fbx
			importScene(RESOURCE_LOCATION "characters/character.fbx",
				"characters/character.bin");

			// Import scene from fbx
			importScene(RESOURCE_LOCATION "scenes/scene.fbx",
				"scenes/scene.bin");

			// Package all compiled resources into one big file
			mara::createPak("C:/Users/marcu/Dev/mara-demo/demo/build/bin/data/assets.pak");
			BASE_TRACE("All assets are compiled and packed!")
		}

		int shutdown() override
		{
			mara::shutdown();

			return 0;
		}

		bool update() override
		{
			if (mara::update(GRAPHICS_DEBUG_TEXT, GRAPHICS_RESET_VSYNC))
			{
				// When this is called (aka: When the window becomes pink), all assets are compiled.
				graphics::frame();

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