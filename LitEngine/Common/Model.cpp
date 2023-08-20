#include "pch.h"
#include "model.h"

#include <fstream> //For file reading
#include <iostream> //For outputting to window for debug purposes
#include <string> //getline
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
	int rowNr = 0;
	for (std::string line; std::getline(fileIn, line);)
	{
		/*
		rowNr++;

		printf("%s ", line.c_str());
		printf("%d\n", rowNr);
		*/

		if(line[1] == 'n')
		{
			nrOfNormals++;
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
	m_Vertices.resize(nrOfVertices);// = new vector<DirectX::XMFLOAT3>(10);
	m_Normals.resize(nrOfNormals);
	m_Colors.resize(nrOfVertices); //Temporary since we have no albedo tex
	m_Indices.resize(1);// m_Indices = new vector<uint32_t>();

	fileIn.close();

	fileIn.open("Harmony.txt");
	string input1;
	string input2;
	string input3;
	//We iterate one more time to feed our vertex & color buffer
	int nrOfRows = 0;
	//First we iterate the vertice positions
	for (int i = 0; i < nrOfVertices; i++) //nrOfVertices*4 because our obj format has 4 words on each line!
	{
		fileIn >> input1; //First will be trash (the v, vn, vt etc)
		fileIn >> input1;
		fileIn >> input2;
		fileIn >> input3;

		m_Vertices[i] = DirectX::XMFLOAT3(stof(input1) * 50.0f, stof(input2) * 50.0f, stof(input3) * 50.0f);
		m_Colors[i] = m_Vertices[i]; //Temporary assign colors to vertex positions
	}
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
