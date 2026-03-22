/******************************************************************************
This file is part of the Newcastle Game Engineering series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#include "Quake3MapLoader.h"

using namespace NCL;
using namespace Rendering;
using namespace Maths;
using namespace idTechLoaders;

const int CURVE_SUBDVISIONS = 10;

Quake3Map::Quake3Map(const std::string& fileName, NCL::Rendering::Mesh* inMesh) : mesh(inMesh) {
	std::ifstream f(fileName, std::ifstream::binary);

	if (!f) {
		return;
	}

	Q3BSPHeader h;

	f.read((char*)&h, sizeof(Q3BSPHeader));

	ParseEntityLump(f, h.entries[0].length, h.entries[0].offset);

	textures	= LoadDataChunk<Q3BSPTexture>(f, h.entries[1].length, h.entries[1].offset);
	planes		= LoadDataChunk<Q3BSPPlane>(f, h.entries[2].length, h.entries[2].offset);
	nodes		= LoadDataChunk<Q3BSPNode>(f, h.entries[3].length, h.entries[3].offset);
	leaves		= LoadDataChunk<Q3BSPLeaf>(f, h.entries[4].length, h.entries[4].offset);
	leafFaces	= LoadDataChunk<Q3BSPLeafFace>(f, h.entries[5].length, h.entries[5].offset);
	leafBrushes = LoadDataChunk<Q3BSPLeafBrush>(f, h.entries[6].length, h.entries[6].offset);
	//Q3BSPModel* models = (Q3BSPModel*)LoadDataChunk(f, h.entries[7].length, h.entries[7].offset);
	//Q3BSPBrush* brushes = (Q3BSPBrush*)LoadDataChunk(f, h.entries[8].length, h.entries[8].offset);
	//Q3BSPBrushSide* brushSides = (Q3BSPBrushSide*)LoadDataChunk(f, h.entries[9].length, h.entries[9].offset);
	meshVertices	= LoadDataChunk<Q3BSPVertex>(f, h.entries[10].length, h.entries[10].offset);
	meshIndices		= LoadDataChunk<Q3BSPMeshVertIndex>(f, h.entries[11].length, h.entries[11].offset);
	//Q3BSPEffect* effects = (Q3BSPEffect*)LoadDataChunk(f, h.entries[12].length, h.entries[12].offset);
	faces		= LoadDataChunk<Q3BSPFace>(f, h.entries[13].length, h.entries[13].offset);
	//Q3BSPLightmap* lightmaps = (Q3BSPLightmap*)LoadDataChunk(f, h.entries[14].length, h.entries[14].offset);
	//Q3BSPLightVolume* volumes = (Q3BSPLightVolume*)LoadDataChunk(f, h.entries[15].length, h.entries[15].offset);

	ParseVisDataLump(f, h.entries[16].length, h.entries[16].offset);

	//ProcessLightmaps(lightmaps, h.entries[14].length / sizeof(Q3BSPLightmap));
	//ProcessTextures(textures, h.entries[1].length / sizeof(Q3BSPTexture));

	uint32_t numVertices = meshVertices.size();
	uint32_t numIndices  = meshIndices.size(); 

	CalculateTotalPatchSize(numVertices, numIndices);

	vPos.resize(numVertices);
	vNorm.resize(numVertices);
	vCol.resize(numVertices);
	vTex.resize(numVertices);
	vTex2.resize(numVertices);
	indices.resize(numIndices);

	CopyVertexData();
	ProcessFaces();
	f.close();

	mesh->SetVertexPositions(vPos);
	mesh->SetVertexNormals(vNorm);
	mesh->SetVertexColours(vCol);
	mesh->SetVertexTextureCoords(vTex);

	mesh->SetVertexIndices(indices);

	visibleFaces.resize((faces.size() / 8) + 1);
}

Quake3Map::~Quake3Map() {

}

void Quake3Map::ParseEntityLump(std::ifstream& f, int size, int offset) {
	f.seekg(offset);
	if (size == 0) {
		return;
	}
	//TODO: the data array ends up with lots of 0s in it, makes parsing a pain...
	char* data = new char[size];

	f.read(data, size);

	std::string temp(data);

	delete data;
}

void  Quake3Map::ParseVisDataLump(std::ifstream& f, int chunkSize, int chunkOffset) {
	f.seekg(chunkOffset);
	if (chunkSize == 0) {
		return;
	}

	f.read((char*)&visData.numVectors, 4);
	f.read((char*)&visData.vectorSize, 4);

	visData.data = new unsigned char[visData.numVectors * visData.vectorSize];

	f.read((char*)visData.data, visData.numVectors * visData.vectorSize);
}

void Quake3Map::ProcessFaces() {
	//new verts/inds from beziers get tacked onto the end of the vertex list
	int vOffset = meshVertices.size();
	int iOffset = meshIndices.size();

	for (int f = 0; f < faces.size(); ++f) {
		Q3BSPFace& face = faces[f];

		SubMesh newSubMesh;

		if (face.type == 2) {
			//Bezier face
			 
			uint32_t vCount = 0;
			uint32_t iCount = 0;

			CalculatePatchSize(vCount, iCount, face);

			newSubMesh.start	= iOffset;
			newSubMesh.count	= iCount;
			newSubMesh.base		= vOffset;

			//now to make the actual new bezier verts
			Q3BSPVertex* controlPoints = &meshVertices[face.firstVertex];

			int numX = (face.patchSize[0] - 1) / 2;
			int numY = (face.patchSize[1] - 1) / 2;

			uint32_t vertOffset = vOffset;
			uint32_t indexOffset = iOffset;

			for (int x = 0; x < numX; ++x) {
				for (int y = 0; y < numY; ++y) {
					int startX = x * 2;
					int startY = y * 2;

					int row0 = (face.patchSize[0] * startY) + startX;

					int row1 = (face.patchSize[0] * (startY + 1)) + startX;
					int row2 = (face.patchSize[0] * (startY + 2)) + startX;

					ComputeQuadBezier(CURVE_SUBDVISIONS, face,
						controlPoints[row0], controlPoints[row0 + 1], controlPoints[row0 + 2],
						controlPoints[row1], controlPoints[row1 + 1], controlPoints[row1 + 2],
						controlPoints[row2], controlPoints[row2 + 1], controlPoints[row2 + 2],
						&vPos[vertOffset], &vNorm[vertOffset]/*, &tangents[currentVertex]*/, &vTex[vertOffset], &vTex2[vertOffset], &indices[indexOffset]
					);
					vertOffset  += CURVE_SUBDVISIONS * CURVE_SUBDVISIONS;
					indexOffset += (CURVE_SUBDVISIONS - 1) * (CURVE_SUBDVISIONS - 1) * 6;
				}
			}
			numProcessedCurveFaces++;
			vOffset += vCount;
			iOffset += iCount;
		}
		else {
			//Normal face
			// I think this was my index hack...
			//for (int j = face.firstVertex; j < face.firstVertex + face.numVertices; ++j) {
			//	vTex2[j].y += (face.lightmapIndex + 1);
			//}
			//Standard faces have their verts in the initial ver
			newSubMesh.start	= face.firstIndex;
			newSubMesh.count	= face.numIndices;
			newSubMesh.base		= face.firstVertex;
		}

		mesh->AddSubMesh(newSubMesh);
	}
}

void Quake3Map::CopyVertexData() {
	for (int i = 0; i < meshIndices.size(); ++i) {
		indices[i] = meshIndices[i].offset;
	}

	for (int i = 0; i < meshVertices.size(); ++i) {
		Q3BSPVertex& v = meshVertices[i];

		//vPos[i]		= Vector3(v.position[0], v.position[2], -v.position[1]);
		vPos[i]		= Vector3(v.position[0], v.position[1], v.position[2]);

		vNorm[i]	= Vector3(v.normal[0], v.normal[2], -v.normal[1]);
		vCol[i]		= Vector4(v.colour[0] / 255.0f, v.colour[1] / 255.0f, v.colour[2] / 255.0f, v.colour[3] / 255.0f);

		vTex[i]		= Vector2(v.diffuseTex[0], v.diffuseTex[1]);
		vTex2[i]	= Vector2(v.lightmapTex[0], v.lightmapTex[1]);
	}
}

void	Quake3Map::CalculatePatchSize(uint32_t& vertexCount, uint32_t& indexCount, const Q3BSPFace& face) {
	if (face.type != 2) {
		return;
	}
	int numX = (face.patchSize[0] - 1) / 2;
	int numY = (face.patchSize[1] - 1) / 2;

	vertexCount += CURVE_SUBDVISIONS * CURVE_SUBDVISIONS * numX * numY;
	indexCount  += ((CURVE_SUBDVISIONS - 1) * (CURVE_SUBDVISIONS - 1) * 6) * numX * numY;
}

void	Quake3Map::CalculateTotalPatchSize(uint32_t& vertexCount, uint32_t& indexCount) {
	uint32_t extraVertCount	= 0;
	uint32_t extraIndCount	= 0;

	for (const Q3BSPFace& face : faces) {
		if (face.type != 2) {
			continue;
		}
		numCountedCurveFaces++;
		CalculatePatchSize(extraVertCount, extraIndCount, face);
	}
	vertexCount += extraVertCount;
	indexCount  += extraIndCount;
}

bool Quake3Map::ComputeQuadBezier(int subdivisionLevel, const Q3BSPFace& face,
	const Q3BSPVertex& cp0, const Q3BSPVertex& cp1, const Q3BSPVertex& cp2,
	const Q3BSPVertex& cp3, const Q3BSPVertex& cp4, const Q3BSPVertex& cp5,
	const Q3BSPVertex& cp6, const Q3BSPVertex& cp7, const Q3BSPVertex& cp8,

	Vector3* verts, Vector3* normals, /*Vector3* tangents,*/ Vector2* texCoords, Vector2* lightCoords, uint32_t* inds
) {

	Vector3 cp0Pos = Vector3(cp0.position[0], cp0.position[1], cp0.position[2]);
	Vector3 cp1Pos = Vector3(cp1.position[0], cp1.position[1], cp1.position[2]);
	Vector3 cp2Pos = Vector3(cp2.position[0], cp2.position[1], cp2.position[2]);

	Vector3 cp3Pos = Vector3(cp3.position[0], cp3.position[1], cp3.position[2]);
	Vector3 cp4Pos = Vector3(cp4.position[0], cp4.position[1], cp4.position[2]);
	Vector3 cp5Pos = Vector3(cp5.position[0], cp5.position[1], cp5.position[2]);

	Vector3 cp6Pos = Vector3(cp6.position[0], cp6.position[1], cp6.position[2]);
	Vector3 cp7Pos = Vector3(cp7.position[0], cp7.position[1], cp7.position[2]);
	Vector3 cp8Pos = Vector3(cp8.position[0], cp8.position[1], cp8.position[2]);

	Vector3 cp0Norm = Vector3(cp0.normal[0], cp0.normal[1], cp0.normal[2]);
	Vector3 cp1Norm = Vector3(cp1.normal[0], cp1.normal[1], cp1.normal[2]);
	Vector3 cp2Norm = Vector3(cp2.normal[0], cp2.normal[1], cp2.normal[2]);

	Vector3 cp3Norm = Vector3(cp3.normal[0], cp3.normal[1], cp3.normal[2]);
	Vector3 cp4Norm = Vector3(cp4.normal[0], cp4.normal[1], cp4.normal[2]);
	Vector3 cp5Norm = Vector3(cp5.normal[0], cp5.normal[1], cp5.normal[2]);

	Vector3 cp6Norm = Vector3(cp6.normal[0], cp6.normal[1], cp6.normal[2]);
	Vector3 cp7Norm = Vector3(cp7.normal[0], cp7.normal[1], cp7.normal[2]);
	Vector3 cp8Norm = Vector3(cp8.normal[0], cp8.normal[1], cp8.normal[2]);


	Vector2 cp0Tex = Vector2(cp0.diffuseTex[0], cp0.diffuseTex[1]);
	Vector2 cp1Tex = Vector2(cp1.diffuseTex[0], cp1.diffuseTex[1]);
	Vector2 cp2Tex = Vector2(cp2.diffuseTex[0], cp2.diffuseTex[1]);

	Vector2 cp3Tex = Vector2(cp3.diffuseTex[0], cp3.diffuseTex[1]);
	Vector2 cp4Tex = Vector2(cp4.diffuseTex[0], cp4.diffuseTex[1]);
	Vector2 cp5Tex = Vector2(cp5.diffuseTex[0], cp5.diffuseTex[1]);

	Vector2 cp6Tex = Vector2(cp6.diffuseTex[0], cp6.diffuseTex[1]);
	Vector2 cp7Tex = Vector2(cp7.diffuseTex[0], cp7.diffuseTex[1]);
	Vector2 cp8Tex = Vector2(cp8.diffuseTex[0], cp8.diffuseTex[1]);

	Vector2 cp0Light = Vector2(cp0.lightmapTex[0], cp0.lightmapTex[1]);
	Vector2 cp1Light = Vector2(cp1.lightmapTex[0], cp1.lightmapTex[1]);
	Vector2 cp2Light = Vector2(cp2.lightmapTex[0], cp2.lightmapTex[1]);

	Vector2 cp3Light = Vector2(cp3.lightmapTex[0], cp3.lightmapTex[1]);
	Vector2 cp4Light = Vector2(cp4.lightmapTex[0], cp4.lightmapTex[1]);
	Vector2 cp5Light = Vector2(cp5.lightmapTex[0], cp5.lightmapTex[1]);

	Vector2 cp6Light = Vector2(cp6.lightmapTex[0], cp6.lightmapTex[1]);
	Vector2 cp7Light = Vector2(cp7.lightmapTex[0], cp7.lightmapTex[1]);
	Vector2 cp8Light = Vector2(cp8.lightmapTex[0], cp8.lightmapTex[1]);

	for (int x = 0; x < subdivisionLevel; ++x) {
		for (int y = 0; y < subdivisionLevel; ++y) {

			float s = x * (1.0f / (subdivisionLevel - 1));
			float t = y * (1.0f / (subdivisionLevel - 1));

			Vector3 s1Pos = EvaluateQuadraticBezier(s, cp0Pos, cp1Pos, cp2Pos);
			Vector3 s2Pos = EvaluateQuadraticBezier(s, cp3Pos, cp4Pos, cp5Pos);
			Vector3 s3Pos = EvaluateQuadraticBezier(s, cp6Pos, cp7Pos, cp8Pos);

			Vector3 s1Norm = EvaluateQuadraticBezier(s, cp0Norm, cp1Norm, cp2Norm);
			Vector3 s2Norm = EvaluateQuadraticBezier(s, cp3Norm, cp4Norm, cp5Norm);
			Vector3 s3Norm = EvaluateQuadraticBezier(s, cp6Norm, cp7Norm, cp8Norm);

			Vector2 s1Tex = EvaluateQuadraticBezier(s, cp0Tex, cp1Tex, cp2Tex);
			Vector2 s2Tex = EvaluateQuadraticBezier(s, cp3Tex, cp4Tex, cp5Tex);
			Vector2 s3Tex = EvaluateQuadraticBezier(s, cp6Tex, cp7Tex, cp8Tex);

			Vector2 s1Light = EvaluateQuadraticBezier(s, cp0Light, cp1Light, cp2Light);
			Vector2 s2Light = EvaluateQuadraticBezier(s, cp3Light, cp4Light, cp5Light);
			Vector2 s3Light = EvaluateQuadraticBezier(s, cp6Light, cp7Light, cp8Light);

			Vector3 tPos	= EvaluateQuadraticBezier(t, s1Pos, s2Pos, s3Pos);
			Vector3 tNorm	= EvaluateQuadraticBezier(t, s1Norm, s2Norm, s3Norm);

			Vector2 tTex	= EvaluateQuadraticBezier(t, s1Tex, s2Tex, s3Tex);
			Vector2 tLight	= EvaluateQuadraticBezier(t, s1Light, s2Light, s3Light);

			tLight.y += (face.lightmapIndex + 1);

			//*verts++		= Vector3(tPos.x, tPos.z, -tPos.y);

			*verts++ = Vector3(tPos.x, tPos.y, tPos.z);

			*normals++		= Vector3(tNorm.x, tNorm.z, -tNorm.y);
			*texCoords++	= tTex;
			*lightCoords++	= tLight;
		}
	}

	for (int x = 0; x < subdivisionLevel - 1; ++x) {
		for (int z = 0; z < subdivisionLevel - 1; ++z) {
			int a = (x * (subdivisionLevel)) + z;
			int b = ((x + 1) * (subdivisionLevel)) + z;
			int c = ((x + 1) * (subdivisionLevel)) + (z + 1);
			int d = (x * (subdivisionLevel)) + (z + 1);
			*inds++ = a;
			*inds++ = d;
			*inds++ = c;

			*inds++ = c;
			*inds++ = b;
			*inds++ = a;
		}
	}
	return true;
}

int	Quake3Map::FindBSPLeaf(const Vector3& pos) {
	int index = 0; 

	Vector3 transPos(pos.x, -pos.z, pos.y);

	transPos = pos;

	while (index >= 0) {//leaves of the tree have negative indices
		const Q3BSPNode&  node	= nodes[index];
		const Q3BSPPlane& bspp	= planes[node.planeIndex];

		float dist = Vector::Dot(Vector3(bspp.x, bspp.y, bspp.z), transPos) - bspp.d;

		index = (dist > 0) ? node.children[0] : node.children[1];
	}
	return ~index;
}

bool Quake3Map::ClusterVisible(int sourceCluster, int destCluster) {
	if (sourceCluster == destCluster) {
		return true;
	}

	int destStart	= sourceCluster * visData.vectorSize;

	int destByte	= destCluster / 8;

	int destBit		= destCluster % 8;

	int check = visData.data[destStart + destByte] & (1 << destBit);

	return (check > 0);
}

bool Quake3Map::IsPositionInMap(const Vector3& pos) {
	int index = FindBSPLeaf(pos);
	Q3BSPLeaf& camLeaf = leaves[index];
	return (index >= 0 && camLeaf.cluster >= 0);
}

void Quake3Map::BuildVisibleSubmeshList(const Vector3& pos, std::vector<uint32_t>& indices) {
	const Q3BSPLeaf& camLeaf = leaves[FindBSPLeaf(pos)];

	if (camLeaf.cluster < 0) {
		return;
	}

	memset(visibleFaces.data(), 0, visibleFaces.size());
	uint32_t lowByte	= INT_MAX;
	uint32_t highByte	= 0;

	for (int i = 0; i < leaves.size(); ++i) {
		const Q3BSPLeaf& testLeaf = leaves[i];
		if (testLeaf.cluster < 0) {
			continue;
		}
		if (ClusterVisible(camLeaf.cluster, testLeaf.cluster)) {
			for (int j = 0; j < testLeaf.numLeafFace; ++j) {
				int actualFace = leafFaces[testLeaf.firstLeafFace + j].faceIndex;

				uint32_t faceByte = actualFace / 8;
				uint32_t faceBit  = actualFace % 8;

				visibleFaces[faceByte] |= (1 << faceBit);

				lowByte		= std::min(faceByte, lowByte);
				highByte	= std::max(faceByte, highByte);
			}
		}
	}
	for (int i = lowByte; i < highByte; ++i) {
		char data = visibleFaces[i];
		if (data) {
			for (int j = 0; j < 8; ++j) {
				if (data & (1 << j)) {
					indices.push_back((i * 8) + j);
				}
			}
		}
	}
}

//void Quake3Map::ProcessTextures(Q3BSPTexture* textures, int numTextures) {
//	vector<string> fileTextures;
//
//	for (int i = 0; i < numTextures; ++i) {
//		Q3BSPTexture& t = textures[i];
//
//		string s((char*)&t.name);
//
//		Texture* n = TextureManager::Instance().AddTexture("/q3/" + s + ".jpg");
//
//		if (n) {
//			fileTextures.push_back("/q3/" + s + ".jpg");
//		}
//
//		if (n == 0) {
//			n = TextureManager::Instance().AddTexture("/q3/" + s + ".tga");
//
//			if (n) {
//				fileTextures.push_back("/q3/" + s + ".tga");
//			}
//		}
//		if (n == 0) {
//			s = TextureFromShaderEntry(s);
//
//			s = StringTools::StripFileExtension(s); //things marked tga actually have jpgs :|
//
//			n = TextureManager::Instance().AddTexture("/q3/" + s + ".tga");
//			if (n) {
//				fileTextures.push_back("/q3/" + s + ".tga");
//			}
//
//			if (!n) {
//				n = TextureManager::Instance().AddTexture("/q3/" + s + ".jpg");
//				if (n) {
//					fileTextures.push_back("/q3/" + s + ".jpg");
//				}
//			}
//
//			if (!n) {
//				s = string((char*)&t.name);
//				Output::Warning("Can't find q3 texture " + s);
//
//				n = TextureManager::Instance().GetTexture("default.png");
//			}
//		}
//		loadedTextures.push_back(n);
//	}
//
//}
//
//void Quake3Map::ProcessLightmaps(Q3BSPLightmap* lightmaps, int numLightmaps) {
//	vector<unsigned char*> mapDatas;
//	for (int i = 0; i < numLightmaps; ++i) {
//		unsigned char* data = (unsigned char*)&lightmaps[i].data;
//		mapDatas.push_back(data);
//	}
//	this->lightmaps = TextureManager::Instance().AddTextureArray("MapLightmaps", mapDatas, 128, 128, 3);
//}

//string	Quake3Map::TextureFromShaderEntry(string input) {
//	//first off, need to get the sub folder
//	string subFolder;
//
//	size_t end = input.find_last_of('/');
//
//	string surface = input.substr(end + 1);
//
//	size_t start = input.find_last_of('/', end - 1);
//
//	subFolder = input.substr(start + 1, (end - start) - 1);
//
//	string filename = TextureManager::Instance().GetManagerFolder() + "q3/scripts/" + subFolder + ".shader";
//
//	ifstream o(filename);
//
//	if (!o) {
//		return "";
//	}
//	//now what :|
//
//	bool foundEntry = false;
//
//	int bracketCount = 0;
//
//	while (!o.eof()) {
//		string temp;
//		getline(o, temp);
//
//		if (foundEntry) {
//			if (temp.find_first_of("{") != string::npos) {
//				bracketCount++;
//				continue;
//			}
//
//			if (temp.length() == 0) {
//				continue;
//			}
//
//			size_t mapat = temp.find("map ");
//			size_t dollar = temp.find("$");
//
//			if (mapat != string::npos && dollar == string::npos) {
//				bool a = true;
//
//				string newname = temp.substr(mapat + 4);
//				return newname;
//			}
//
//			if (temp.find_first_of("}") != string::npos) {
//				bracketCount--;
//				if (bracketCount == 0) {
//					return ""; //at the end...
//				}
//				continue;
//			}
//		}
//		if (!foundEntry && temp.find(input) != string::npos) {
//			foundEntry = true;
//		}
//	}
//	return subFolder;
//}

//const vector<Texture*>& Quake3Map::GetTextureSet() {
//	return loadedTextures;
//}

//void	Quake3Map::DebugDrawNodes() {
//	for (int i = 0; i < numNodes; ++i) {
//		Q3BSPNode& n = nodes[i];
//
//		Vector3 mins((float)n.mins[0], (float)n.mins[2], (float)-n.mins[1]);
//		Vector3 maxs((float)n.maxs[0], (float)n.maxs[2], (float)-n.maxs[1]);
//
//		Renderer::DefaultDebugStream()->DrawDebugAABB(DEBUGDRAW_PERSPECTIVE, mins, maxs, Vector3(0, 0, 1));
//	}
//}
//
//void	Quake3Map::DebugDrawLeaves() {
//	for (int i = 0; i < numLeaves; ++i) {
//		Q3BSPLeaf& l = leaves[i];
//
//		if (l.cluster == -1) {
//			continue;
//		}
//
//		Vector3 mins((float)l.mins[0], (float)l.mins[2], (float)-l.mins[1]);
//		Vector3 maxs((float)l.maxs[0], (float)l.maxs[2], (float)-l.maxs[1]);
//
//		Renderer::DefaultDebugStream()->DrawDebugAABB(DEBUGDRAW_PERSPECTIVE, mins, maxs, Vector3(0, 0, 1));
//	}
//}