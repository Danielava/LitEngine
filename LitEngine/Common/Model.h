#pragma once
//#ifndef MODEL_H //Same as #pragma once
//#define MODEL_H

#include "..\Common\DeviceResources.h" //DX:: namespace
#include "DirectXHelper.h" //ThrowIfFailed()

#include <vector>
#include <string> //getline
#include <mutex> //lock_guard, mutex
#include <algorithm>
#include <DirectXTex.h>
//#include <DirectXTex/DirectXTexWic.cpp> //For LoadFromWICFile
//Guide on how to add new library to your project (you have to add the vcproj solution to your sol by rightclicking in the sol explorer) 
//See "Using project-to-project references": https://github.com/microsoft/DirectXTex/wiki/DirectXTex

//THIS FIXED THE IMPORTING DirectXTex library and linking it: https://directxtex2.rssing.com/chan-6426152/all_p34.html
/*
	In your application's project, right-click on the Project and use "References...", then "Add New Reference...", and then check the DirectXTex project name and click OK. For a Windows Store app, you need to set Reference Assembly Output to false since DirectXTex is a static C++ library and not a WinRT component.

	In your application's project settings, on the "C++ / General" page set Configuration to "All Configurations", set Platform to "All Platforms", and then add the relative path to DirectXTex; to the Additional Include Directories properties. Click Apply.
*/

using namespace std;

class Model
{
public:

	Model();
	Model(const std::shared_ptr<DX::DeviceResources>& deviceResources, bool x);
	Model(const char* filepath);
	~Model() {}

	size_t GetNrOfVertices() { return m_Vertices.size(); }
	vector<int>* GetNrOfVerticesListPointer() { return &m_NrOfVerticesPerComponent; }
	vector<int> GetNrOfVerticesList() { return m_NrOfVerticesPerComponent; }

	vector<DirectX::XMFLOAT3> m_Vertices;
	vector<DirectX::XMFLOAT3> m_Normals;
	vector<DirectX::XMFLOAT2> m_UVs;
	vector<DirectX::XMFLOAT3> m_Colors;
	vector<DirectX::XMFLOAT3> m_VertexListTemp;

	vector<DirectX::XMUINT3> m_Indices;

	struct TextureInfo
	{
		ID3D12Resource* m_AlbedoTex;

		UINT imageSize;
		UINT RowPitch; //Texture Pitch which is stored in here, is the width of the texture times the size of a single pixel in bytes.
		UINT SlicePitch; //
	};

	struct ModelComponentTextures
	{
		ID3D12Resource* m_AlbedoTex;
		ID3D12Resource* m_NormalTex;
		ID3D12Resource* m_MaterialTex;
		
		TextureInfo m_AlbedoTexInfo;
		TextureInfo m_NormalTexInfo;
		TextureInfo m_MaterialTexInfo;
	};

	ID3D12Resource* m_TextureBufferUploadHeap;
	vector<ModelComponentTextures> m_Textures;


	//The latest and greatest: Support for multiple components in 1 model
	/*
		We will have a separate list, holding how many vertices each component has.
		E.g m_NrOfVerticesPerComponent[0] = 815 //Component 0 has 815 vertices.
		m_NrOfVerticesPerComponent[1] = 1023 //Component 1 has 1023 vertices.

		This will be used to properly issue drawcalls. BUT! The model vertices will all be stored in m_Vertices,
		and we will use m_NrOfVerticesPerComponent to properly index into the m_Vertices for each component drawcall.

		This way I think it's easier to setup the one single vertex buffer and index buffer and not multiple ones.
	*/
	vector<string> m_MaterialNames;
	vector<int> m_NrOfVerticesPerComponent;

private:
	stringstream GetModifiedLine(string line, char separator = ' ');
	bool LineShouldBeSkipped(string line);

	void ProcessModelVertices(ifstream* fileIn, int nrOfVertices);
	void ProcessModelNormals(ifstream* fileIn, int nrOfNormals);
	void ProcessModelUVs(ifstream* fileIn, int nrOfUVs);
	void ProcessModelTriangles(ifstream* fileIn, int nrOfTriangles);

	void PushBackVertex(string line);
	void PushBackTriangle(string line);
	void PushBackUVs(string line);


	std::mutex ms_TextureCacheMutex; //To make sure only one thread accesses the texture cache at one time.
	void LoadTextureFromFile(const std::shared_ptr<DX::DeviceResources>& deviceResources, string filepath);
};

//#endif