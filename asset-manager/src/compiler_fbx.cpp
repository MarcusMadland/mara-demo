#include "compiler_fbx.h"

#include <fbxsdk.h>
#include <ufbx.h>
#include <mrender/bgfx.h>

#include "mengine/mengine.h"

namespace am {

	void OLDProcessNode(const am::AssetDetails& _assetDetails, FbxNode* _node)
	{
		FbxMesh* mesh = _node->GetMesh();

		if (mesh) 
		{
			// Remove bad polygons
			mesh->RemoveBadPolygons();

			// FBX data
			U32 numVertices = mesh->GetControlPointsCount();
			FbxVector4* vertices = mesh->GetControlPoints();

			U32 numIndices = mesh->GetPolygonVertexCount();
			int* indices = mesh->GetPolygonVertices();

			// Engine data
			mengine::MeshVertex* geomVertices = new mengine::MeshVertex[numVertices];
			U16* geomIndices = new U16[numIndices];

			// Process data
			for (U32 i = 0; i < numVertices; ++i)
			{
				geomVertices[i].positions[0] = (F32)mesh->GetControlPointAt(i)[0];
				geomVertices[i].positions[1] = (F32)mesh->GetControlPointAt(i)[1];
				geomVertices[i].positions[2] = (F32)mesh->GetControlPointAt(i)[2];

				if (geomVertices[i].positions[0] == 0.0f || geomVertices[i].positions[0] == FBXSDK_FLOAT_MAX ||
					geomVertices[i].positions[1] == 0.0f || geomVertices[i].positions[1] == FBXSDK_FLOAT_MAX ||
					geomVertices[i].positions[2] == 0.0f || geomVertices[i].positions[2] == FBXSDK_FLOAT_MAX)
				{
					bx::debugPrintf("Position %u is null\n");
				}
			}

			for (U32 i = 0; i < numIndices; i++)
			{
				geomIndices[i] = indices[i];
			}

			// Create Asset
			bx::DefaultAllocator allocator;
			mengine::GeometryAsset geometry;
			geometry.m_vertexMemory = bgfx::makeRef(geomVertices,  numVertices * sizeof(mengine::MeshVertex), &allocator);
			geometry.m_indexMemory = bgfx::makeRef(geomIndices, numIndices * sizeof(U16), &allocator);

			// Serialize Asset
			bx::FilePath outputPath = _assetDetails.outputDirectoryPath;
			outputPath.join(_assetDetails.name.toUtf8().constData());
			outputPath.join(".bin", false); // @todo make custom type

			if (!geometry.serialize(outputPath))
			{
				bx::debugPrintf("Failed to serialize asset\n");
			}

			delete[] geomVertices;
			delete[] geomIndices;

			return; // @todo Support multiple geometries from one file
		}

		for (int i = 0; i < _node->GetChildCount(); i++)
		{
			OLDProcessNode(_assetDetails, _node->GetChild(i));
		}
	}

	void OLDcompileGeometryFromFBX(const am::AssetDetails& _assetDetails, U16 _index)
	{
		// Init FBX SDK and manager
		FbxManager* manager = FbxManager::Create();
		FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
		manager->SetIOSettings(ios);
		FbxImporter* importer = FbxImporter::Create(manager, "GeometryImporter");
		if (!importer->Initialize(_assetDetails.sourceFilePath.getCPtr(), -1, manager->GetIOSettings()))
		{
			bx::debugPrintf("FBX SDK failed to find fbx file.");
			return;
		}

		// Import FBX scene
		FbxScene* scene = FbxScene::Create(manager, "");
		importer->Import(scene);
		importer->Destroy();

		// Access the root node of the FBX scene
		FbxNode* rootNode = scene->GetRootNode();

		// Process all nodes
		if (rootNode)
		{
			OLDProcessNode(_assetDetails, rootNode);
		}

		manager->Destroy();
	}

	bool operator==(const mengine::MeshVertex& lhs, const mengine::MeshVertex& rhs)
	{
		return memcmp(&lhs, &rhs, sizeof(mengine::MeshVertex)) == 0;
	}

	void compileGeometryFromFBX(const am::AssetDetails& _assetDetails, U16 _index)
	{
		// Load FBX
		ufbx_load_opts opts{};
		ufbx_error err;
		ufbx_scene* scene = ufbx_load_file(_assetDetails.sourceFilePath.getCPtr(), &opts, &err);
		if (!scene)
		{
			bx::debugPrintf("Failed to load: %s\n", err.description.data);
			return;
		}

		// Create a data structure to hold unique vertices and indices.
		std::vector<mengine::MeshVertex> uniqueVertices;
		std::vector<U16> indices;

		// Handle Mesh
		for (size_t i = 0; i < scene->nodes.count; i++)
		{
			ufbx_node* node = scene->nodes.data[i];
			if (node->is_root) continue;

			// Handle Mesh
			if (node->mesh)
			{
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

							mengine::MeshVertex vertex;
							vertex.positions[0] = (F32)node->mesh->vertex_position[index].x;
							vertex.positions[1] = (F32)node->mesh->vertex_position[index].y;
							vertex.positions[2] = (F32)node->mesh->vertex_position[index].z;

							if (node->mesh->vertex_uv.exists)
							{
								vertex.uv[0] = (F32)node->mesh->vertex_uv.values[node->mesh->vertex_uv.indices[index]].x;
								vertex.uv[1] = (F32)node->mesh->vertex_uv.values[node->mesh->vertex_uv.indices[index]].y;
							}

							if (node->mesh->vertex_normal.exists)
							{
								vertex.normals[0] = (F32)node->mesh->vertex_normal.values[node->mesh->vertex_normal.indices[index]].x;
								vertex.normals[1] = (F32)node->mesh->vertex_normal.values[node->mesh->vertex_normal.indices[index]].y;
								vertex.normals[2] = (F32)node->mesh->vertex_normal.values[node->mesh->vertex_normal.indices[index]].z;
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
				bx::DefaultAllocator allocator;
				mengine::GeometryAsset geometry;
				geometry.m_vertexMemory = bgfx::makeRef(uniqueVertices.data(), uniqueVertices.size() * sizeof(mengine::MeshVertex), &allocator);
				geometry.m_indexMemory = bgfx::makeRef(indices.data(), indices.size() * sizeof(U16), &allocator);

				// Serialize Asset
				bx::FilePath outputPath = _assetDetails.outputDirectoryPath;
				outputPath.join(_assetDetails.name.toUtf8().constData());
				outputPath.join(".bin", false); // @todo make custom type

				if (!geometry.serialize(outputPath))
				{
					bx::debugPrintf("Failed to serialize asset\n");
				}

				break; // @todo Currently imports first mesh it finds, add support for multiple meshes
			}
		}

		ufbx_free_scene(scene);
	}

}