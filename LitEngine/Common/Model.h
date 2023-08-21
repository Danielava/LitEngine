#pragma once
//#ifndef MODEL_H //Same as #pragma once
//#define MODEL_H

#include <vector>
#include <string> //getline

using namespace std;

class Model
{
public:
	Model();
	Model(bool x);
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
};

//#endif