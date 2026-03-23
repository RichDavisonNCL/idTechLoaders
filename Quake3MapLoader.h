/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial SeriesThis file is part of the Newcastle Game Engineering series
Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../NCLCoreClasses/Mesh.h"
#include "../NCLCoreClasses/Vector.h"

namespace NCL::idTechLoaders {
	using namespace NCL::Maths;

	struct Q3BSPDirEntry {
		int offset;
		int length;
	};

	struct Q3BSPHeader {
		unsigned char	magicNumber[4];
		int		version;
		Q3BSPDirEntry entries[17];
	};

	struct Q3BSPTexture {
		unsigned char name[64];
		int flags;
		int contents;
	};

	struct Q3BSPPlane {
		float x;
		float y;
		float z;
		float d;
	};

	struct Q3BSPNode {
		int planeIndex;

		int children[2];
		int mins[3];
		int maxs[3];
	};

	struct Q3BSPLeaf {
		int cluster;
		int area;
		int mins[3];
		int maxs[3];
		int firstLeafFace;
		int numLeafFace;
		int firstLeafBrush;
		int numLeafBrush;
	};

	struct Q3BSPLeafFace {
		int faceIndex;
	};

	struct Q3BSPLeafBrush {
		int brushIndex;
	};

	struct Q3BSPModel {
		float mins[3];
		float maxs[3];
		int firstFace;
		int numFaces;
		int firstBrush;
		int numBrushes;
	};

	struct Q3BSPBrush {
		int firstBrushSide;
		int numBrushSides;
		int texture;
	};

	struct Q3BSPBrushSide {
		int plane;
		int texture;
	};

	struct Q3BSPVertex {
		float position[3];
		float diffuseTex[2];
		float lightmapTex[2];
		float normal[3];
		unsigned char  colour[4];
	};

	struct Q3BSPMeshVertIndex {
		int offset;
	};

	struct Q3BSPEffect {
		unsigned char	name[64];
		int		brush;
		int		unknown;
	};

	struct Q3BSPFace {
		int		texture;
		int		effect;
		int		type;
		int		firstVertex;
		int		numVertices;
		int		firstIndex;
		int		numIndices;
		int		lightmapIndex;
		int		lightmapStart[2];
		int		lightmapSize[2];
		float	lightmapOrigin[3];
		float	lightmapSVector[3];
		float	lightmapTVector[3];
		float	normals[3];
		int		patchSize[2];
	};

	struct Q3BSPLightmap {
		unsigned char data[128][128][3];
	};

	struct Q3BSPLightVolume {
		unsigned char ambient[3];
		unsigned char directional[3];
		unsigned char dir[3];
	};

	struct Q3BSPVisData {
		int numVectors;
		int vectorSize;

		unsigned char* data;
	};

	struct Quake3FaceMaterial {
		uint32_t diffuseID	= 0;
		uint32_t lightmapID = 0;
	};

	typedef std::function<void(char* data, uint32_t x, uint32_t y, uint32_t bpp)> LightmapHandlingFunc;

	class Quake3Map{
	public:
		Quake3Map(const std::string& fileName, NCL::Rendering::Mesh* mesh, LightmapHandlingFunc lightmapHandler);
		~Quake3Map();

		const NCL::Rendering::Mesh* GetMesh() const {
			return mesh;
		}
		bool IsPositionInMap(const Vector3& pos);

		bool BuildVisibleSubmeshList(const Vector3& pos, std::vector<uint32_t>& indices);
		
		const std::vector<Quake3FaceMaterial>& GetTextureSet() const { return faceMaterials; }

	protected:
		NCL::Rendering::Mesh* mesh;

		void ParseEntityLump(std::ifstream& f, int size, int offset);
		void ParseVisDataLump(std::ifstream& f, int size, int offset);

		template<typename T>
		std::vector<T> LoadDataChunk(std::ifstream& f, int chunkSize, int chunkOffset) {
			if (chunkSize == 0) {
				return {};
			}
			
			size_t count = chunkSize / sizeof(T);
			std::vector<T> data;
			data.resize(count);

			f.seekg(chunkOffset);
			f.read((char*)data.data(), chunkSize);
			return data;
		}

		void CopyVertexData();
		void ProcessFaces();

		void	CalculateTotalPatchSize(uint32_t& vertexCount, uint32_t& indexCount);
		void	CalculatePatchSize(uint32_t& vertexCount, uint32_t& indexCount, const Q3BSPFace& face);

		//void ProcessTextures();

		void ProcessLightmaps(LightmapHandlingFunc handler);

		bool ComputeQuadBezier(int subdivisionLevel,
			const Q3BSPVertex& cp0, const Q3BSPVertex& cp1, const Q3BSPVertex& cp2,
			const Q3BSPVertex& cp3, const Q3BSPVertex& cp4, const Q3BSPVertex& cp5,
			const Q3BSPVertex& cp6, const Q3BSPVertex& cp7, const Q3BSPVertex& cp8,
			uint32_t indexOffset, 
			Vector3* verts, Vector3* normals, Vector2* texCoords, Vector2* lightCoords, uint32_t* inds
		);

		int		FindBSPLeaf(const Vector3& pos);
		bool	ClusterVisible(int sourceCluster, int destCluster);
		
		//string	TextureFromShaderEntry(string input);

	protected:
		std::vector<Q3BSPNode>		nodes;
		std::vector<Q3BSPLeaf>		leaves;
		std::vector<Q3BSPPlane>		planes;
		std::vector<Q3BSPFace>		faces;
		std::vector<Q3BSPLeafFace>	leafFaces;
		std::vector<Q3BSPLeafBrush> leafBrushes;

		std::vector<Q3BSPTexture>	textures;
		std::vector<Q3BSPLightmap>	lightmaps;

		std::vector<Q3BSPVertex>		meshVertices;
		std::vector<Q3BSPMeshVertIndex> meshIndices;

		std::vector< Quake3FaceMaterial> faceMaterials;

		Q3BSPVisData	visData;

		std::vector<Vector3>	vPos;
		std::vector<Vector3>	vNorm;
		std::vector<Vector4>	vCol;
		std::vector<Vector2>	vTex;
		std::vector<Vector2>	vTex2;
		std::vector<uint32_t>	indices;

		int numCountedCurveFaces	= 0;
		int numProcessedCurveFaces	= 0;

		std::vector<char> visibleFaces;
	};
}