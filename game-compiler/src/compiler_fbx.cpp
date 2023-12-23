#include "compiler_fbx.h"

#include "ufbx.h"

#include <mengine/mengine.h>
#include <mrender/bgfx.h>

#include <vector>

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

mengine::GeometryCreate createResourceFromFBX(const bx::FilePath& _filePath, U16 _meshIndex, const bx::FilePath& _vfp)
{
	// Load FBX
	ufbx_load_opts opts{};
	ufbx_error err;
	ufbx_scene* scene = ufbx_load_file(_filePath.getCPtr(), &opts, &err);
	if (!scene)
	{
		BX_TRACE("Failed to load: %s", err.description.data)
		return mengine::GeometryCreate();
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
			mengine::createResource(geometry, _vfp);

			ufbx_free_scene(scene);

			return geometry;
		}
		break;
	}

	ufbx_free_scene(scene);

	BX_TRACE("Could not find mehs and _meshIndex %u", _meshIndex);
	return mengine::GeometryCreate();
}

