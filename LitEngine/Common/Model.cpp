#include "pch.h"
#include "model.h"

#include <fstream> //For file reading
#include <iostream> //For outputting to window for debug purposes
#include <sstream> // std::stringstream
#include <functional> //For storing functions in variables e.g std::function<int()> myFunction = [] { return 0; }

/*
	Daniel Tutorial:
	Visual studio won't be able to see any cpp files you add so they won't be part of any compiles.
	What you have to do is to add this cpp file to the VS solution so it knows to compile it along with everything else.

	So simply drag and drop those newly created files to the VS solution window.
*/

/*
	Tutorial2:

	To make printf print stuff to the output window:

	Bring up the projects properties, go to linker->system->subsystem and change it to the third option, CONSOLE. That should do it

	SAME WITH ANY 3DMODELS YOU HAVE!!!!
*/

/*
	Tutorial3:

	So WEIRD!!
	If you want the file reading to work in Debug you must put your .txt file in:
	C:\Users\Daniel\GraphicsProjects\LitEngine\x64\Debug\LitEngine\AppX
*/

#include <direct.h> // _getcwd

// get the number of bits per pixel for a dxgi format
	// Taken from https://github.com/microsoft/Windows-Machine-Learning/blob/master/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/D3D12Quad.cpp
static int GetDXGIFormatBitsPerPixel(DXGI_FORMAT dxgiFormat)
{
	switch (dxgiFormat) {
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return 128;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return 64;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return 64;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return 32;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return 32;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return 32;
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		return 32;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return 32;
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return 16;
	case DXGI_FORMAT_B5G6R5_UNORM:
		return 16;
	case DXGI_FORMAT_R32_FLOAT:
		return 32;
	case DXGI_FORMAT_R16_FLOAT:
		return 16;
	case DXGI_FORMAT_R16_UNORM:
		return 16;
	case DXGI_FORMAT_R8_UNORM:
		return 8;
	case DXGI_FORMAT_A8_UNORM:
		return 8;
	}
	return 0;
}

std::string current_working_directory()
{
	char* cwd = _getcwd(0, 0); // **** microsoft specific ****, Gets current path, we need the path to know which filepath our txt will exist in.
	std::string working_directory(cwd);
	std::free(cwd);
	return working_directory;
}
/*
std::string get_current_dir() {
	char buff[FILENAME_MAX]; //create string buffer to hold path
	_getcwd(buff, FILENAME_MAX);
	string current_working_dir(buff);
	return current_working_dir;
}
*/

/*
	Returns the filestream line and separates the words with the condition provided in separator
*/
stringstream Model::GetModifiedLine(string line, char separator)
{
	std::stringstream ss(line); //To separate string into words by given condition

	//A convoluted way to format the line into something we want, in this case separating it by ' '
	std::string words;
	std::string resultString = "";
	while (getline(ss, words, separator))
	{
		resultString += words;
		resultString += " ";
	}

	std::stringstream ss2(resultString);

	return ss2;
}

//Skip comments and other things for now
bool Model::LineShouldBeSkipped(std::string line)
{
	//Skip comments and other things for now
	if (line[0] == '#' || line[0] == 'o' || line[0] == 'm' || line[0] == 's' || line[0] == 'u') //u = usemtl
	{
		return true;
	}
	return false;
}

Model::Model(const char* filepath)
{
	ifstream fileIn;
	//fileIn.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	//string pp = "C:\\Users\\Daniel\\GraphicsProjects\\LitEngine\\LitEngine\\Assets\\3DModels\\Harmony\\Harmony.txt";
	//string pp = "C:/Users/Daniel/GraphicsProjects/LitEngine/LitEngine/Assets/3DModels/Harmony/Harmony.txt";
	fileIn.open("Harmony2.txt");
	//fileIn.open("C:\\Users\\Daniel\\GraphicsProjects\\LitEngine\\LitEngine\\Assets\\3DModels\\Harmony\\Harmony.txt");

	int nrOfVertices = 0;
	int nrOfNormals = 0;
	int nrOfUvCoords = 0;
	int nrOfTriangles = 0; //Nr of indices?
	int rowNr = 0;
	for (std::string line; std::getline(fileIn, line);)
	{
		if (rowNr != 0 && line[0] == 'o')
		{
			break;
		}

		if(LineShouldBeSkipped(line))
		{
			continue;
		}

		if(line[1] == 'n')
		{
			nrOfNormals++;
		}
		else if(line[1] == 't')
		{
			nrOfUvCoords++;
		}
		else if(line[0] == 'f')
		{
			nrOfTriangles++;
		}
		else
		{
			nrOfVertices++;
		}

		rowNr++;
	}

	//printf("p: %s", current_working_directory().c_str()); //printf is a c thing, not c++, that's why you can't print strings with it. BUT you can write strings if you use c_str();
	//Gives the following path: C:\Users\Daniel\GraphicsProjects\LitEngine\x64\Debug\LitEngine\AppX
	printf("Nr of vertices: %d\n", nrOfVertices);
	printf("Nr of normals: %d\n", nrOfNormals);
	printf("Nr of UV coords: %d\n", nrOfUvCoords);
	printf("Nr of Triangles: %d\n", nrOfTriangles);
	printf("Nr of Rows: %d\n", rowNr);
	m_Vertices.resize(nrOfVertices);// = new vector<DirectX::XMFLOAT3>(10);
	m_Normals.resize(nrOfNormals);
	m_UVs.resize(nrOfUvCoords);
	m_Colors.resize(nrOfVertices); //Temporary since we have no albedo tex
	m_Indices.resize(1);// m_Indices = new vector<uint32_t>();

	m_VertexListTemp.resize(nrOfTriangles*3);

	fileIn.close();

	//We iterate one more time to feed our vertex & color buffer
	fileIn.open("Harmony.txt");
	
	ProcessModelVertices(&fileIn, nrOfVertices);
	ProcessModelNormals(&fileIn, nrOfNormals);
	ProcessModelUVs(&fileIn, nrOfUvCoords);
	ProcessModelTriangles(&fileIn, nrOfTriangles);

	//Temporary dangerous operation!!
	m_Vertices = m_VertexListTemp;
	m_Colors = m_Vertices;
}

//Default constructor creates the dummy cube
Model::Model()
{
	using namespace DirectX;

	m_Vertices.resize(8);// = new vector<DirectX::XMFLOAT3>(10);
	m_Normals.resize(8);
	m_Colors.resize(8); //Temporary since we have no albedo tex
	m_Indices.resize(1);

	m_Vertices =
	{
		XMFLOAT3(-0.5f, -0.5f, -0.5f),
		XMFLOAT3(-0.5f, -0.5f,  0.5f),
		XMFLOAT3(-0.5f,  0.5f, -0.5f),
		XMFLOAT3(-0.5f,  0.5f,  0.5f),
		XMFLOAT3(0.5f, -0.5f, -0.5f),
		XMFLOAT3(0.5f, -0.5f,  0.5f),
		XMFLOAT3(0.5f,  0.5f, -0.5f),
		XMFLOAT3(0.5f,  0.5f,  0.5f),
	};

	m_Colors =
	{
		XMFLOAT3(1.0f, 0.0f, 0.0f),
		XMFLOAT3(1.0f, 0.0f, 0.0f),
		XMFLOAT3(1.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 1.0f),
		XMFLOAT3(1.0f, 0.0f, 0.0f),
		XMFLOAT3(1.0f, 0.0f, 1.0f),
		XMFLOAT3(1.0f, 1.0f, 0.0f),
		XMFLOAT3(1.0f, 1.0f, 1.0f),
	};
}

/*
	Testing with putting all model vertices into one huge vertex buffer and processing all of it in Sample3DSceneRenderer.cpp.

	To make this work out. I think we just have to put all vertices in the list and that's it?
	For that we need a loop which loops over nr of componenets this model has.

	That will be indicated by the letter o.
*/
Model::Model(const std::shared_ptr<DX::DeviceResources>& deviceResources, bool x)
{
	ifstream fileIn;
	//fileIn.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	//string pp = "C:\\Users\\Daniel\\GraphicsProjects\\LitEngine\\LitEngine\\Assets\\3DModels\\Harmony\\Harmony.txt";
	//string pp = "C:/Users/Daniel/GraphicsProjects/LitEngine/LitEngine/Assets/3DModels/Harmony/Harmony.txt";
	//fileIn.open("Harmony2.txt");
	//fileIn.open("C:\\Users\\Daniel\\GraphicsProjects\\LitEngine\\LitEngine\\Assets\\3DModels\\Harmony\\Harmony.txt");

	fileIn.open("Harmony2.txt");
	//fileIn.open("Zelda.txt");

	int nrOfVertices = 0;
	int nrOfNormals = 0;
	int nrOfUvCoords = 0;
	int nrOfTriangles = 0; //Nr of indices?
	int rowNr = 0;
	int nrOfModelComponenets = -1;

	auto sumListElements = [](vector<int> list) //Helper function
	{
		int sum = 0;
		for (auto it = list.begin(); it != list.end(); it++)
		{
			sum += *it;
		}
		return sum;
	};

	for (std::string line; std::getline(fileIn, line);)
	{
		rowNr++;

		if (line[0] == 'o')
		{
			if(nrOfModelComponenets >= 0)
			{
				m_NrOfVerticesPerComponent.push_back(0);
				m_NrOfVerticesPerComponent[nrOfModelComponenets] = nrOfVertices - sumListElements(m_NrOfVerticesPerComponent);
			}
			nrOfModelComponenets++;
		}

		if (line[0] == 'u') //usemtl
		{
			string input1;

			std::stringstream lineStream = GetModifiedLine(line, ' ');

			lineStream >> input1; //First will be trash (the v, vn, vt etc)
			lineStream >> input1;

			MaterialInfo info;
			info.name = input1;

			m_MaterialNames.push_back(info);
		}

		if (LineShouldBeSkipped(line))
		{
			continue;
		}

		if (line[1] == 'n')
		{
			nrOfNormals++;
			PushBackNormal(line);
		}
		else if (line[1] == 't')
		{
			nrOfUvCoords++;
			PushBackUVs(line);
		}
		else if (line[0] == 'f')
		{
			PushBackTriangle(line); //This will actually disect the triangle's 3 vertices and register their indices. But the nr of 'f' does accurately represent nr of triangles.
			nrOfTriangles++;
		}
		else
		{
			PushBackVertex(line);
			nrOfVertices++;
		}
	}

	//We need to register the last component vertices right here
	if(nrOfModelComponenets > 0)
	{
		m_NrOfVerticesPerComponent.push_back(0);
		m_NrOfVerticesPerComponent[nrOfModelComponenets] = nrOfVertices - sumListElements(m_NrOfVerticesPerComponent);
	}

	//printf("p: %s", current_working_directory().c_str()); //printf is a c thing, not c++, that's why you can't print strings with it. BUT you can write strings if you use c_str();
	//Gives the following path: C:\Users\Daniel\GraphicsProjects\LitEngine\x64\Debug\LitEngine\AppX
	printf("Nr of vertices: %d\n", nrOfVertices);
	printf("Nr of normals: %d\n", nrOfNormals);
	printf("Nr of UV coords: %d\n", nrOfUvCoords);
	printf("Nr of Triangles: %d\n", nrOfTriangles);
	printf("Nr of Rows: %d\n", rowNr);

	m_VertexListTemp.resize(nrOfTriangles * 3);
	
	vector<DirectX::XMFLOAT2> m_UVListTemp;
	m_UVListTemp.resize(nrOfTriangles * 3);
	
	vector<DirectX::XMFLOAT3> m_NormalListTemp;
	m_NormalListTemp.resize(nrOfTriangles * 3);

	fileIn.close();

	for (int i = 0; i < m_Indices.size(); i++)
	{
		m_VertexListTemp[i] = m_Vertices[m_Indices[i].x-1]; //DANIEL: REMEMBER the -1 here it's very important!! It's not 0 indexed.. (smh)
		m_UVListTemp[i] = m_UVs[m_Indices[i].y-1]; //Something is wrong here... m_UVs seem to be missing uvs? OMG.. seems like the middle elem (y) is uv...
		m_NormalListTemp[i] = m_Normals[m_Indices[i].z-1];
	}

	//We iterate one more time to feed our vertex & color buffer
	/*
	fileIn.open("Harmony.txt");

	ProcessModelVertices(&fileIn, nrOfVertices);
	ProcessModelNormals(&fileIn, nrOfNormals);
	ProcessModelUVs(&fileIn, nrOfUvCoords);
	ProcessModelTriangles(&fileIn, nrOfTriangles);
	*/
	//Temporary dangerous operation!!
	m_Vertices = m_VertexListTemp;
	m_UVs = m_UVListTemp;
	m_Normals = m_NormalListTemp;

	//Open textures
	//TODO: You need to loop here and open all textures stored in m_Materials.

	LoadMaterials(deviceResources);

	//filepath = "3DModels\\m_body_alb.png"; //Also WORKED!!
	//LoadTextureFromFile(filepath);
}

void Model::PushBackVertex(string line)
{
	string input1;
	string input2;
	string input3;

	std::stringstream lineStream = GetModifiedLine(line, ' ');

	lineStream >> input1; //First will be trash (the v, vn, vt etc)
	lineStream >> input1;
	lineStream >> input2;
	lineStream >> input3;

	m_Vertices.push_back(DirectX::XMFLOAT3(stof(input1) * 50.0f, stof(input2) * 50.0f, stof(input3) * 50.0f));
	//m_Vertices.push_back(DirectX::XMFLOAT3(stof(input1), stof(input2), stof(input3)));
}

void Model::PushBackNormal(string line)
{
	string input1;
	string input2;
	string input3;

	std::stringstream lineStream = GetModifiedLine(line, ' ');

	lineStream >> input1; //First will be trash (the v, vn, vt etc)
	lineStream >> input1;
	lineStream >> input2;
	lineStream >> input3;

	m_Normals.push_back(DirectX::XMFLOAT3(stof(input1), stof(input2), stof(input3)));
}

void Model::PushBackUVs(string line)
{
	string input1;
	string input2;

	std::stringstream lineStream = GetModifiedLine(line, ' ');

	lineStream >> input1; //First will be trash (the v, vn, vt etc)
	lineStream >> input1;
	lineStream >> input2;

	m_UVs.push_back(DirectX::XMFLOAT2(stof(input1), 1.0f - stof(input2)));
}

void Model::PushBackTriangle(string line)
{
	string index1;
	string index2;
	string index3;

	std::stringstream lineStream = GetModifiedLine(line, ' ');
	lineStream >> index1; //Throw away the 'f'

	for (int j = 0; j < 3; j++)
	{
		std::string indicesString;
		lineStream >> indicesString;

		std::stringstream indicesLineStream = GetModifiedLine(indicesString, '/');

		indicesLineStream >> index1; //vertexIdx
		indicesLineStream >> index2; //NormalIdx
		indicesLineStream >> index3; //Uv idx??
		m_Indices.push_back(DirectX::XMUINT3(stoi(index1), stoi(index2), stoi(index3))); //First index1 is the vertex index, 2 is normal index etc
	}
}


void Model::ProcessModelVertices(ifstream* fileIn, int nrOfVertices)
{
	string input1;
	string input2;
	string input3;
	//We iterate one more time to feed our vertex & color buffer
	//First we iterate the vertice positions
	for (int i = 0; i < nrOfVertices; i++)
	{
		std::string line;
		std::getline(*fileIn, line);
		while (LineShouldBeSkipped(line))
		{
			std::getline(*fileIn, line);
		}

		std::stringstream lineStream = GetModifiedLine(line, ' ');

		lineStream >> input1; //First will be trash (the v, vn, vt etc)
		lineStream >> input1;
		lineStream >> input2;
		lineStream >> input3;

		m_Vertices[i] = DirectX::XMFLOAT3(stof(input1) * 50.0f, stof(input2) * 50.0f, stof(input3) * 50.0f);
		m_Colors[i] = m_Vertices[i]; //Temporary assign colors to vertex positions
	}
}

void Model::ProcessModelNormals(ifstream* fileIn, int nrOfNormals)
{
	string input1;
	string input2;
	string input3;
	//Next up is the normals
	for (int i = 0; i < nrOfNormals; i++)
	{
		std::string line;
		std::getline(*fileIn, line);
		while (LineShouldBeSkipped(line))
		{
			std::getline(*fileIn, line);
		}

		std::stringstream lineStream = GetModifiedLine(line, ' ');

		lineStream >> input1; //First will be trash (the v, vn, vt etc)
		lineStream >> input1;
		lineStream >> input2;
		lineStream >> input3;

		m_Normals[i] = DirectX::XMFLOAT3(stof(input1), stof(input2), stof(input3));
	}
}
void Model::ProcessModelUVs(ifstream* fileIn, int nrOfUVs)
{
	string input1;
	string input2;
	string input3;

	//UV coords
	for (int i = 0; i < nrOfUVs; i++)
	{
		std::string line;
		std::getline(*fileIn, line);
		while (LineShouldBeSkipped(line))
		{
			std::getline(*fileIn, line);
		}

		std::stringstream lineStream = GetModifiedLine(line, ' ');

		lineStream >> input1; //First will be trash (the v, vn, vt etc)
		lineStream >> input1;
		lineStream >> input2;

		m_UVs[i] = DirectX::XMFLOAT2(stof(input1), stof(input2));
	}
}
void Model::ProcessModelTriangles(ifstream* fileIn, int nrOfTriangles)
{
	string input1;
	string input2;
	string input3;

	//Throw away some garbage line (But these might be useful for later!!)
	std::string line;
	std::getline(*fileIn, line); //s 1
	std::getline(*fileIn, line); //usemtl M_Body

	//Triangles (can we create index buffer from this as well?)
	for (int i = 0; i < nrOfTriangles; i++)
	{
		std::string line;
		std::getline(*fileIn, line);
		while (LineShouldBeSkipped(line))
		{
			std::getline(*fileIn, line);
		}

		std::stringstream lineStream = GetModifiedLine(line, ' ');
		lineStream >> input1; //Throw away the 'f'

		for (int j = 0; j < 3; j++)
		{
			std::string indicesString;
			lineStream >> indicesString;

			std::stringstream indicesLineStream = GetModifiedLine(indicesString, '/');

			std::string vertexIndexString;
			indicesLineStream >> vertexIndexString;
			int vertexIndex = stoi(vertexIndexString);
			m_VertexListTemp[j + i * 3] = m_Vertices[vertexIndex - 1]; //Here we're basically picking from our vertex list and adding the according to how the 'f' tells us they should be sorted
		}
	}
}

static void StringToWString(std::wstring& ws, const std::string& s)
{
	std::wstring wsTmp(s.begin(), s.end());
	ws = wsTmp;
}

//Using DirectXTex to load images into DX12 format. Tutorial: https://www.3dgep.com/learning-directx-12-4/
void Model::LoadTextureFromFile(const std::shared_ptr<DX::DeviceResources>& deviceResources, string filepath, int materialIndex)
{
	std::lock_guard<std::mutex> lock(ms_TextureCacheMutex); //Note, we don't have a texture cache yet so this is useless!
	
	DirectX::TexMetadata metadata;
	DirectX::ScratchImage* scratchImage = new DirectX::ScratchImage();
	
	{
		std::wstring path;
		StringToWString(path, filepath);
		const wchar_t* wpath = path.c_str();
		HRESULT hr = LoadFromWICFile(wpath, DirectX::WIC_FLAGS_FORCE_RGB, &metadata, *scratchImage, nullptr);

		//DirectX::LoadFromTGAFile(wpath, &metadata, *scratchImage);

		if (FAILED(hr))
		{
			// Clean up for partial success before here
			//return hr; // Must keep passing the error code back all the way to the main loop
			assert("Texture creation failed!");
		}

		//hr = CreateShaderResourceView(mD3DSystem->GetDevice11(), image->GetImages(), image->GetImageCount(), imageMetadata, srv);

		//Great resource on how to create Texture (ID3D12Resource) object https://alextardif.com/D3D11To12P3.html

		bool is3DTexture = metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D;
		D3D12_RESOURCE_DESC textureDesc{};
		textureDesc.Format = metadata.format;
		textureDesc.Width = (uint32)metadata.width;
		textureDesc.Height = (uint32)metadata.height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = is3DTexture ? (uint16)metadata.depth : (uint16)metadata.arraySize;
		textureDesc.MipLevels = (uint16)metadata.mipLevels;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = is3DTexture ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.Alignment = 0;

		D3D12_HEAP_PROPERTIES defaultProperties;
		defaultProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		defaultProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		defaultProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		defaultProperties.CreationNodeMask = 0;
		defaultProperties.VisibleNodeMask = 0;

		ID3D12Resource* newTextureResource = NULL;
		
		//CreateCommittedResource(&defaultProperties, D3D12_HEAP_FLAG_NONE, &textureDesc,
		//	D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&newTextureResource));

		/*
			Daniel very important: The reason this texture was black is because it has no content in it.
			This is simply allocating some data in video memory (the default heap), we must now upload
			our IMAGE DATA to this heap.
		*/

		DX::ThrowIfFailed(deviceResources->GetD3DDevice()->CreateCommittedResource(
			&defaultProperties,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, //It starts of as a Copy dest since we want to upload/copy data to this with the upload heap! Later, this must be Barrier'd into a pixel resource
			NULL, //Used for RT and DS/Stencil buffers
			IID_PPV_ARGS(&newTextureResource)
		));

		//TUTORIAL: Upload heap https://www.braynzarsoft.net/viewtutorial/q16390-directx-12-textures-from-file
		//Upload heap, upload the image data to our newTextureResource
		{
			UINT64 textureUploadBufferSize = 0;
			// this function gets the size an upload buffer needs to be to upload a texture to the gpu.
			// each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
			// eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
			//textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
			deviceResources->GetD3DDevice()->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

			// now we create an upload heap to upload our texture to the GPU
			hr = deviceResources->GetD3DDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
				D3D12_HEAP_FLAG_NONE, // no flags
				&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
				D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
				nullptr,
				IID_PPV_ARGS(&m_TextureBufferUploadHeap[materialIndex]));
			if (FAILED(hr))
			{
				//TODO: Som error message
			}
		}

		//Now we have a default heap that the pixel shader can read from (video memory data), we use the upload heap to upload the tex data to the default heap.
		//OBS: I decided to do this in Sample3DSceneRenderer.cpp because we don't have access to the commandList here atm..
		//But we will store some data here for the Sample3DSceneRenderer.cpp to use

		ModelComponentTextures textures;

		textures.m_AlbedoTex = newTextureResource;
		textures.m_NormalTex = nullptr;
		textures.m_MaterialTex = nullptr;

		int bitsPerPixel = GetDXGIFormatBitsPerPixel(metadata.format); // number of bits per pixel
		int bytesPerRow = ((uint32)metadata.width * bitsPerPixel) / 8; // number of bytes in each row of the image data
		int imageSize = bytesPerRow * (uint32)metadata.height; // total image size in bytes

		const DirectX::Image* pImage = scratchImage->GetImages();
		/*
			const Image* pImages = scratchImage.GetImages();

			for ( int i = 0; i < scratchImage.GetImageCount(); ++i ){

			auto& subresource = subresources[i];
			subresource.RowPitch = pImages[i].rowPitch;
			subresource.SlicePitch = pImages[i].slicePitch;
			subresource.pData = pImages[i].pixels;
		*/
		//Good tutorial for how to apply these params https://logins.github.io/graphics/2020/09/20/D3D12TexturesPart2.html
		//textures.m_AlbedoTexInfo.imageSize = pImage->pixels;
		//textures.m_AlbedoTexInfo.RowPitch = pImage->rowPitch; //((uint32)metadata.width * bitsPerPixel);//(uint32)metadata.width* bitsPerPixel;// (uint32)metadata.width * bitsPerPixel;
		//textures.m_AlbedoTexInfo.SlicePitch = pImage->slicePitch;// textures.m_AlbedoTexInfo.RowPitch* (uint32)metadata.height;
		textures.m_AlbedoTexInfo.m_Image = pImage;

		m_Textures.push_back(textures);
	}
}

void Model::LoadMaterials(const std::shared_ptr<DX::DeviceResources>& deviceResources)
{
	m_TextureBufferUploadHeap.resize(m_MaterialNames.size());
	int currentMaterialIndex = 0;

	ifstream fileIn;
	fileIn.open("Harmony.mtl");

	for (std::string line; std::getline(fileIn, line);)
	{
		string input1;

		std::stringstream lineStream = GetModifiedLine(line, ' ');

		lineStream >> input1;

		if (input1.compare("newmtl") == 0)
		{
			string materialName;
			lineStream >> materialName;

			for (int j = 0; j < m_MaterialNames.size(); j++)
			{
				if (m_MaterialNames[j].name.compare(materialName) == 0)
				{
					currentMaterialIndex = j;
					break;
				}
			}
		}
		else if (input1.compare(0, 3, "map") == 0)
		{
			//We're grabbing all textures (they are all marked by "map" at the beginning line)
			string texPath;
			lineStream >> texPath;
			m_MaterialNames[currentMaterialIndex].texturePaths.push_back(texPath);
		}
	}

	
	for (int i = 0; i < m_MaterialNames.size(); i++)
	{
		string filepath = "3DModels/";
		filepath = filepath + m_MaterialNames[i].texturePaths[0]; //texturePaths[0] should always be albedo tex!
		LoadTextureFromFile(deviceResources, filepath, i);
	}
	

	//string filepath = "3DModels/";
	//filepath = filepath + "M_Body_Alb.png";
	//LoadTextureFromFile(deviceResources, filepath);
	//string filepath = "3DModels/m_body_alb.png"; //WORKED!!
	//string filepath = "3DModels/m_cap_alb.png";
	//string filepath = "3DModels/m_clothes_alb.png";
	//string filepath = "3DModels/m_clothes_nrm.png";
	//string filepath = "3DModels/m_cap_ao.png";
	//string filepath = "3DModels/m_hair_alb.png";
	//LoadTextureFromFile(deviceResources, filepath);
}