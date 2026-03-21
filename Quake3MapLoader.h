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

	class Quake3Map{
	public:
		Quake3Map(const std::string& fileName, NCL::Rendering::Mesh* mesh);
		~Quake3Map();

		const NCL::Rendering::Mesh* GetMesh() const {
			return mesh;
		}

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

		void BuildVertexData(int startVertex, int startIndex);
		void BuildSubmeshes(int startVertex, int startIndex);

		void ProcessPatches(Q3BSPFace* faces, int faceCount);

		void	CalculateTotalPatchSize(int& vertexCount, int& indexCount, Q3BSPFace* faces, int faceCount);
		void	CalculatePatchSize(int& vertexCount, int& indexCount, Q3BSPFace* faces, int index);

		//void ProcessTextures(Q3BSPTexture* textures, int numTextures);
		//void ProcessLightmaps(Q3BSPLightmap* lightmaps, int numLightmaps);

		bool ComputeQuadBezier(int subdivisionLevel, Q3BSPFace& face,
			Q3BSPVertex& cp0, Q3BSPVertex& cp1, Q3BSPVertex& cp2,
			Q3BSPVertex& cp3, Q3BSPVertex& cp4, Q3BSPVertex& cp5,
			Q3BSPVertex& cp6, Q3BSPVertex& cp7, Q3BSPVertex& cp8,
			int currentVertex, int currentIndex,
			Vector3* verts, Vector3* normals, Vector3* tangents, Vector2* texCoords, Vector2* lightCoords, uint32_t* inds
		);

		//void NewDetermineDraws(Camera& c, vector<Q3BSPFace*>& visibleFaces, Matrix4& modelMat);

		int	FindLeaf(const Vector3& pos);
		int	FindLeafByAABB(const Vector3& pos);

		bool ClusterVisible(int sourceCluster, int destCluster);
		bool IsPositionInMap(const Vector3& pos);

		void	ComputeMaximumSizes();

		//string	TextureFromShaderEntry(string input);

		//const vector<Texture*>& GetTextureSet();

	protected:
		std::vector<Q3BSPTexture>	textures;
		std::vector<Q3BSPNode>		nodes;
		std::vector<Q3BSPLeaf>		leaves;
		std::vector<Q3BSPPlane>		planes;
		std::vector<Q3BSPFace>		faces;
		std::vector<Q3BSPLeafFace>	leafFaces;
		std::vector<Q3BSPLeafBrush> leafBrushes;

		std::vector<Q3BSPVertex>		meshVertices;
		std::vector<Q3BSPMeshVertIndex> meshIndices;

		Q3BSPVisData	visData;

		std::vector<Vector3>	vPos;
		std::vector<Vector3>	vNorm;
		std::vector<Vector4>	vCol;
		std::vector<Vector2>	vTex;
		std::vector<Vector2>	vTex2;
		std::vector<uint32_t>	indices;

		int* visibleFaceIndices;
		int  lowVisibleFace;
		int	 highVisibleFace;
		int  visibleFaceClock;

		unsigned int maxIndexCount;
		unsigned int maxLeafCount;

		int numCountedCurveFaces	= 0;
		int numProcessedCurveFaces	= 0;
	};
}