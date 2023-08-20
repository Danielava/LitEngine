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

	vector<DirectX::XMFLOAT3> m_Vertices;
	vector<DirectX::XMFLOAT3> m_Normals;
	vector<DirectX::XMFLOAT2> m_UVs;
	vector<DirectX::XMFLOAT3> m_Colors;
	vector<DirectX::XMFLOAT3> m_VertexListTemp;

	vector<DirectX::XMUINT3> m_Indices;
private:
	stringstream GetModifiedLine(string line, char separator = ' ');
	bool LineShouldBeSkipped(string line);

	void ProcessModelVertices(ifstream* fileIn, int nrOfVertices);
	void ProcessModelNormals(ifstream* fileIn, int nrOfNormals);
	void ProcessModelUVs(ifstream* fileIn, int nrOfUVs);
	void ProcessModelTriangles(ifstream* fileIn, int nrOfTriangles);

	void Model::PushBackVertex(string line);
	void Model::PushBackTriangle(string line);
};

//#endif