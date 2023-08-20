#pragma once
//#ifndef MODEL_H //Same as #pragma once
//#define MODEL_H

#include <vector>

using namespace std;

class Model
{
public:
	Model();
	Model(const char* filepath);
	~Model() {}

	size_t GetNrOfVertices() { return m_Vertices.size(); }

	vector<DirectX::XMFLOAT3> m_Vertices;
	vector<DirectX::XMFLOAT3> m_Normals;
	vector<DirectX::XMFLOAT3> m_Colors;
	vector<uint32_t> m_Indices;
private:
};

//#endif