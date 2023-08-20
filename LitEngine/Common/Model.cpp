#include "pch.h"
#include "model.h"

#include <fstream> //For file reading
#include <iostream> //For outputting to window for debug purposes
#include <sstream> // std::stringstream
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
	fileIn.open("Harmony.txt");
	//fileIn.open("C:\\Users\\Daniel\\GraphicsProjects\\LitEngine\\LitEngine\\Assets\\3DModels\\Harmony\\Harmony.txt");

	int nrOfVertices = 0;
	int nrOfNormals = 0;
	int nrOfUvCoords = 0;
	int nrOfTriangles = 0; //Nr of indices?
	int rowNr = 0;
	for (std::string line; std::getline(fileIn, line);)
	{
		/*
		rowNr++;

		printf("%s ", line.c_str());
		printf("%d\n", rowNr);
		*/
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
	}

	//printf("p: %s", current_working_directory().c_str()); //printf is a c thing, not c++, that's why you can't print strings with it. BUT you can write strings if you use c_str();
	//Gives the following path: C:\Users\Daniel\GraphicsProjects\LitEngine\x64\Debug\LitEngine\AppX
	printf("Nr of vertices: %d\n", nrOfVertices);
	printf("Nr of normals: %d\n", nrOfNormals);
	printf("Nr of UV coords: %d\n", nrOfUvCoords);
	printf("Nr of Triangles: %d\n", nrOfTriangles);
	m_Vertices.resize(nrOfVertices);// = new vector<DirectX::XMFLOAT3>(10);
	m_Normals.resize(nrOfNormals);
	m_UVs.resize(nrOfUvCoords);
	m_Colors.resize(nrOfVertices); //Temporary since we have no albedo tex
	m_Indices.resize(1);// m_Indices = new vector<uint32_t>();

	vector<DirectX::XMFLOAT3> m_VertexListTemp(nrOfTriangles*3);

	fileIn.close();

	fileIn.open("Harmony.txt");
	string input1;
	string input2;
	string input3;
	//We iterate one more time to feed our vertex & color buffer
	int nrOfRows = 0;
	//First we iterate the vertice positions
	for (int i = 0; i < nrOfVertices; i++)
	{
		std::string line;
		std::getline(fileIn, line);
		while(LineShouldBeSkipped(line))
		{
			std::getline(fileIn, line);
		}

		std::stringstream lineStream = GetModifiedLine(line, ' ');

		lineStream >> input1; //First will be trash (the v, vn, vt etc)
		lineStream >> input1;
		lineStream >> input2;
		lineStream >> input3;

		m_Vertices[i] = DirectX::XMFLOAT3(stof(input1) * 50.0f, stof(input2) * 50.0f, stof(input3) * 50.0f);
		m_Colors[i] = m_Vertices[i]; //Temporary assign colors to vertex positions
	}
	
	//Next up is the normals
	for (int i = 0; i < nrOfNormals; i++)
	{
		std::string line;
		std::getline(fileIn, line);
		while (LineShouldBeSkipped(line))
		{
			std::getline(fileIn, line);
		}

		std::stringstream lineStream = GetModifiedLine(line, ' ');

		lineStream >> input1; //First will be trash (the v, vn, vt etc)
		lineStream >> input1;
		lineStream >> input2;
		lineStream >> input3;

		m_Normals[i] = DirectX::XMFLOAT3(stof(input1), stof(input2), stof(input3));
	}

	//UV coords
	for (int i = 0; i < nrOfUvCoords; i++)
	{
		std::string line;
		std::getline(fileIn, line);
		while (LineShouldBeSkipped(line))
		{
			std::getline(fileIn, line);
		}

		std::stringstream lineStream = GetModifiedLine(line, ' ');

		lineStream >> input1; //First will be trash (the v, vn, vt etc)
		lineStream >> input1;
		lineStream >> input2;

		m_UVs[i] = DirectX::XMFLOAT2(stof(input1), stof(input2));
	}

	//Throw away some garbage line (But these might be useful for later!!)
	std::string line;
	std::getline(fileIn, line); //s 1
	std::getline(fileIn, line); //usemtl M_Body

	//Triangles (can we create index buffer from this as well?)
	for (int i = 0; i < nrOfTriangles; i++)
	{
		std::string line;
		std::getline(fileIn, line);
		while (LineShouldBeSkipped(line))
		{
			std::getline(fileIn, line);
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
			m_VertexListTemp[j+i*3] = m_Vertices[vertexIndex-1];
		}
	}

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
