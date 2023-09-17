#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"
#include <ppltasks.h>
#include <synchapi.h>

using namespace LitEngine;

using namespace Concurrency;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Storage;

// Indices into the application state map.
Platform::String^ AngleKey = "Angle";
Platform::String^ TrackingKey = "Tracking";

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources, Model* model) :
	m_loadingComplete(false),
	m_radiansPerSecond(XM_PIDIV4),	// rotate 45 degrees per second
	m_angle(0),
	m_tracking(false),
	m_mappedConstantBuffer(nullptr),
	m_deviceResources(deviceResources)
{
	LoadState();
	ZeroMemory(&m_constantBufferData, sizeof(m_constantBufferData));

	CreateDeviceDependentResources(model);
	CreateWindowSizeDependentResources();
}

Sample3DSceneRenderer::~Sample3DSceneRenderer()
{
	m_constantBuffer->Unmap(0, nullptr);
	m_mappedConstantBuffer = nullptr;
}

void Sample3DSceneRenderer::CreateDeviceDependentResources(Model* model)
{
	auto d3dDevice = m_deviceResources->GetD3DDevice();

	// Create a root signature with a single constant buffer slot.
	{
		//Daniel: I added this sampler here as well which will be part of our root signature!
		//Create a static sampler
		
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;// D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0; //Important here!! Setting the shader register
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		
		//FKING AMAZING TUTORIAL ON HOW TO BIND MULTIPLE RESOURCES IN DESCR HEAP!!!
		//https://stackoverflow.com/questions/55628161/how-to-bind-textures-to-different-register-in-dx12

		CD3DX12_DESCRIPTOR_RANGE rangeCB;
		rangeCB.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE rangeSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);


		CD3DX12_ROOT_PARAMETER rootParams[2]; //One for each resource

		//Daniel: Whoa!! What is this???
		//range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //Changed second param 1->2 //Since we have a CB and SRV!
		rootParams[0].InitAsDescriptorTable(1, &rangeCB, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParams[1].InitAsDescriptorTable(1, &rangeSRV, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // Only the input assembler stage needs access to the constant buffer.
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS; //Daniel: Nahnah we want our desc table in the pixel shader!

		CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
		int nrOfRootParameters = 2; //Daniel: Does this has to be changed?
		descRootSignature.Init(_countof(rootParams), rootParams, 1, &sampler, rootSignatureFlags); //Daniel: Ah, the sampler is inputed in 3rd and 4rth param (3rd = nr of samplers)

		ComPtr<ID3DBlob> pSignature;
		ComPtr<ID3DBlob> pError;
		DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf()));
		DX::ThrowIfFailed(d3dDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
        NAME_D3D12_OBJECT(m_rootSignature);
	}

	// Load shaders asynchronously.
	auto createVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso").then([this](std::vector<byte>& fileData) {
		m_vertexShader = fileData;
	});

	auto createPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso").then([this](std::vector<byte>& fileData) {
		m_pixelShader = fileData;
	});

	// Create the pipeline state once the shaders are loaded.
	auto createPipelineStateTask = (createPSTask && createVSTask).then([this]() {

		static const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
		state.InputLayout = { inputLayout, _countof(inputLayout) };
		state.pRootSignature = m_rootSignature.Get();
        state.VS = CD3DX12_SHADER_BYTECODE(&m_vertexShader[0], m_vertexShader.size());
        state.PS = CD3DX12_SHADER_BYTECODE(&m_pixelShader[0], m_pixelShader.size());
		state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		state.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT; //Fixes the vertex winding order being wrong
		state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		state.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		state.SampleMask = UINT_MAX;
		state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		state.NumRenderTargets = 1;
		state.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
		state.DSVFormat = m_deviceResources->GetDepthBufferFormat();
		state.SampleDesc.Count = 1;

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&m_pipelineState)));

		// Shader data can be deleted once the pipeline state is created.
		m_vertexShader.clear();
		m_pixelShader.clear();
	});

	// Create and upload cube geometry resources to the GPU.
	auto createAssetsTask = createPipelineStateTask.then([this, model]() {
		auto d3dDevice = m_deviceResources->GetD3DDevice();

		// Create a command list.
		DX::ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_deviceResources->GetCommandAllocator(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
        NAME_D3D12_OBJECT(m_commandList);

		/*
			TODO: This is where we should read from a textFile and input right here.
			Pos, color, uvs!

			Daniel task: There is one more thing you'll have to do!
			Task 1)
			I) So this is apparently how it works!

			A big model will use one huge vertex and index buffer, but each model part will
			offset into this huge Vertex & Index buffer!

			DrawIndexedInstanced(x,y,z,w); //One of these xyzw is actually the offset into the vertex buffer
		*/
		// Cube vertices. Each vertex has a position and a color.
		
		VertexPositionColor cubeVertices[] =
		{
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
		};
		
		//Great resource:
		//Tutorial on how to pass std::vertex data into vertexBuffer https://computergraphics.stackexchange.com/questions/1649/dx12-vertex-buffer-incorrect
		/*
			The main reason for the crash when using std::vertex as input to the vertex buffer, was because I was computing the vertexBufferSize manually.
			which leads us to 19560 bytes. But we instead want to assign sizeof(modelVertices) as vertexBufferSize.

			What we assign there is incompatible with the pointer we later assign to the VertexBuffer pData which likely points
			to a larger/(smaller?) chunk of memory than what we specified in our manual calculation.
		*/
		
		vector<VertexPositionColor> modelVertices(model->GetNrOfVertices());
		for (int i = 0; i < modelVertices.size(); i++)
		{
			modelVertices[i] = { model->m_Vertices[i], model->m_Normals[i], model->m_UVs[i]};
		}

		const size_t MODEL_BYTE_SIZE = sizeof(DirectX::XMFLOAT3) * model->GetNrOfVertices() * 2 + sizeof(DirectX::XMFLOAT2) * model->GetNrOfVertices(); //2 = because we have vertices and colors!
		const UINT vertexBufferSize = (UINT)MODEL_BYTE_SIZE;
		
		//19560 is the byte size for our model! It's correct!
		//Cube vertices is 192 bytes (6 floats * 4 bytes * 8 elements = 192)
		//const UINT vertexBufferSize = sizeof(cubeVertices);

		// Create the vertex buffer resource in the GPU's default heap and copy vertex data into it using the upload heap.
		// The upload resource must not be released until after the GPU has finished using it.
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUpload;

		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize); //Daniel: Assigning vertexBufferSize here is correct!
		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&vertexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&vertexBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBufferUpload)));

        NAME_D3D12_OBJECT(m_vertexBuffer);

		// Upload the vertex buffer to the GPU.
		{
			D3D12_SUBRESOURCE_DATA vertexData = {};
			vertexData.pData = reinterpret_cast<BYTE*>(modelVertices.data()); //Ok this fixed the crash. With std::vector you'll have to do .data()
			vertexData.RowPitch = vertexBufferSize;
			vertexData.SlicePitch = vertexData.RowPitch;

			UpdateSubresources(m_commandList.Get(), m_vertexBuffer.Get(), vertexBufferUpload.Get(), 0, 0, 1, &vertexData);

			CD3DX12_RESOURCE_BARRIER vertexBufferResourceBarrier =
				CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			m_commandList->ResourceBarrier(1, &vertexBufferResourceBarrier);
		}

		// Load mesh indices. Each trio of indices represents a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes 0, 2 and 1 from the vertex buffer compose the
		// first triangle of this mesh.
		unsigned short cubeIndices[] =
		{
			0, 2, 1, // -x
			1, 2, 3,

			4, 5, 6, // +x
			5, 7, 6,

			0, 1, 5, // -y
			0, 5, 4,

			2, 6, 7, // +y
			2, 7, 3,

			0, 4, 6, // -z
			0, 6, 2,

			1, 3, 7, // +z
			1, 7, 5,
		};

		const UINT indexBufferSize = sizeof(cubeIndices);

		// Create the index buffer resource in the GPU's default heap and copy index data into it using the upload heap.
		// The upload resource must not be released until after the GPU has finished using it.
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUpload;

		CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&indexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer)));

		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&indexBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&indexBufferUpload)));

		NAME_D3D12_OBJECT(m_indexBuffer);

		// Upload the index buffer to the GPU.
		{
			D3D12_SUBRESOURCE_DATA indexData = {};
			indexData.pData = reinterpret_cast<BYTE*>(cubeIndices);
			indexData.RowPitch = indexBufferSize;
			indexData.SlicePitch = indexData.RowPitch;

			UpdateSubresources(m_commandList.Get(), m_indexBuffer.Get(), indexBufferUpload.Get(), 0, 0, 1, &indexData);

			CD3DX12_RESOURCE_BARRIER indexBufferResourceBarrier =
				CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			m_commandList->ResourceBarrier(1, &indexBufferResourceBarrier);
		}

		// Create a descriptor heap for the constant buffers.
		//Daniel: Use this for your textures as well cause you can only have one of each type in pipeline!
		//Type is the D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = 4;// DX::c_frameCount; //Daniel: Should be 4 here, we have 3 constant buffers lol and 1 SRV
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			// This flag indicates that this descriptor heap can be bound to the pipeline and that descriptors contained in it can be referenced by a root table.
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			DX::ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvHeap)));

            NAME_D3D12_OBJECT(m_cbvHeap);
		}

		CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(DX::c_frameCount * c_alignedConstantBufferSize);
		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&constantBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer)));

        NAME_D3D12_OBJECT(m_constantBuffer);

		// Create constant buffer views to access the upload buffer.
		D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = m_constantBuffer->GetGPUVirtualAddress();
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
		m_cbvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (int n = 0; n < DX::c_frameCount; n++) //DANIEL: WOW!!! MISSED THIS! CONST Buffer is created and inputted in DescTable 3 times!!!! c_frameCount = 3
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
			desc.BufferLocation = cbvGpuAddress;
			desc.SizeInBytes = c_alignedConstantBufferSize;
			d3dDevice->CreateConstantBufferView(&desc, cbvCpuHandle);

			cbvGpuAddress += desc.SizeInBytes;
			cbvCpuHandle.Offset(m_cbvDescriptorSize);
		}

		//Daniel: Bind your SRV!
		/*
			Very important here is to node the cbvCpuHandle.Offset(m_cbvDescriptorSize);
			It has been offseted so you can use it again and it should properly point to
			a new empty location!
		*/
		//Daniel: Now create your SRV now
		ID3D12Resource* albedoTex = model->m_Textures[0].m_AlbedoTex;
		{
			//CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart(), 3, m_cbvDescriptorSize); //Daniel: You need a handle!

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			//Weird that the ThrowIfFailed didn't work here...
			d3dDevice->CreateShaderResourceView(
				model->m_Textures[0].m_AlbedoTex,
				&srvDesc,//metadata.IsCubemap() ? &shaderResourceViewDesc : NULL, //For textures, it will automatically take care of the SRVDesc for us.
				cbvCpuHandle
			);

			//Offset the Handle again so you can use it for even more textures in the DescrHeap
			//m_cbvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			//cbvCpuHandle.Offset(m_cbvDescriptorSize); Daniel: I don't think this is needed..

			// Upload the Texture data to the GPU.
			// OBS: It would probably be best if this code was in Model.cpp where the upload heap is created but we don't have access to the commandList there.. so let's do here instead.
			{

				//BYTE* imageData = (BYTE*)malloc(model->m_Textures[0].m_AlbedoTexInfo.imageSize);

				D3D12_SUBRESOURCE_DATA textureData = {};
				textureData.pData = model->m_Textures[0].m_AlbedoTexInfo.m_Image->pixels;//model->m_Textures[0].m_AlbedoTex;
				textureData.RowPitch = model->m_Textures[0].m_AlbedoTexInfo.m_Image->rowPitch;
				textureData.SlicePitch = model->m_Textures[0].m_AlbedoTexInfo.m_Image->slicePitch;

				// Now we copy the upload buffer contents to the default heap
				const UINT subresourceCount = 1;//texDesc.DepthOrArraySize * texDesc.MipLevels;
				UpdateSubresources(m_commandList.Get(), model->m_Textures[0].m_AlbedoTex, model->m_TextureBufferUploadHeap, 0, 0, subresourceCount, &textureData);

				// transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
				m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(model->m_Textures[0].m_AlbedoTex, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
			}

			//Important note: Right here we can apparently delete the image once we're done with it (in this tutorial once its uploaded to the gpu which is right here!)
		}

		// Map the constant buffers.
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		DX::ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedConstantBuffer)));
		ZeroMemory(m_mappedConstantBuffer, DX::c_frameCount * c_alignedConstantBufferSize);
		// We don't unmap this until the app closes. Keeping things mapped for the lifetime of the resource is okay.

		// Close the command list and execute it to begin the vertex/index buffer copy into the GPU's default heap.
		DX::ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Create vertex/index buffer views.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(VertexPositionColor);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;// sizeof(modelVertices); //This is the issue here!! sizeof(cubeVertices) = 192, sizeof(modelVertices) = 32..

		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.SizeInBytes = sizeof(cubeIndices);
		m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

		// Wait for the command list to finish executing; the vertex/index buffers need to be uploaded to the GPU before the upload resources go out of scope.
		m_deviceResources->WaitForGpu();
	});

	createAssetsTask.then([this]() {
		m_loadingComplete = true;
	});
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	D3D12_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
	m_scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height)};

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.7f, 3.0f, 0.0f };
	static const XMVECTORF32 at = { -0.0f, -0.0f, -10.0f, 0.0f }; //0.0f, -0.1f, 0.0f, 0.0f
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f }; // 0.0f, 1.0f, 0.0f, 0.0f

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (m_loadingComplete)
	{
		if (!m_tracking)
		{
			// Rotate the cube a small amount.
			m_angle += static_cast<float>(timer.GetElapsedSeconds()) * m_radiansPerSecond;

			Rotate(m_angle);
		}

		// Update the constant buffer resource.
		UINT8* destination = m_mappedConstantBuffer + (m_deviceResources->GetCurrentFrameIndex() * c_alignedConstantBufferSize);
		memcpy(destination, &m_constantBufferData, sizeof(m_constantBufferData));
	}
}

// Saves the current state of the renderer.
void Sample3DSceneRenderer::SaveState()
{
	auto state = ApplicationData::Current->LocalSettings->Values;

	if (state->HasKey(AngleKey))
	{
		state->Remove(AngleKey);
	}
	if (state->HasKey(TrackingKey))
	{
		state->Remove(TrackingKey);
	}

	state->Insert(AngleKey, PropertyValue::CreateSingle(m_angle));
	state->Insert(TrackingKey, PropertyValue::CreateBoolean(m_tracking));
}

// Restores the previous state of the renderer.
void Sample3DSceneRenderer::LoadState()
{
	auto state = ApplicationData::Current->LocalSettings->Values;
	if (state->HasKey(AngleKey))
	{
		m_angle = safe_cast<IPropertyValue^>(state->Lookup(AngleKey))->GetSingle();
		state->Remove(AngleKey);
	}
	if (state->HasKey(TrackingKey))
	{
		m_tracking = safe_cast<IPropertyValue^>(state->Lookup(TrackingKey))->GetBoolean();
		state->Remove(TrackingKey);
	}
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader.
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void Sample3DSceneRenderer::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking()
{
	m_tracking = false;
}

/*
	Creates our SRV's etc for this model, it should store them right in this class
*/
void Sample3DSceneRenderer::CreateShaderResources(const std::shared_ptr<DX::DeviceResources>& deviceResources, Model* model)
{
	//Albedo texture is in model->m_Textures[0].m_AlbedoTex;
	/*
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
	//Weird that the ThrowIfFailed didn't work here...
	m_deviceResources->GetD3DDevice()->CreateShaderResourceView(
			model->m_Textures[0].m_AlbedoTex,
			NULL,//metadata.IsCubemap() ? &shaderResourceViewDesc : NULL, //For textures, it will automatically take care of the SRVDesc for us.
			m_cbvHeap->GetCPUDescriptorHandleForHeapStart()
	);
	*/

	//How to insert this SRV into our desc heap!
	//Our commited resource is model->m_Textures[0].m_AlbedoTex.

	D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = model->m_Textures[0].m_AlbedoTex->GetGPUVirtualAddress();
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
	m_cbvDescriptorSize = m_deviceResources->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int n = 0; n < DX::c_frameCount; n++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
		m_deviceResources->GetD3DDevice()->CreateShaderResourceView(model->m_Textures[0].m_AlbedoTex, NULL, cbvCpuHandle);

		//cbvGpuAddress += shaderResourceViewDesc.SizeInBytes;
		//cbvCpuHandle.Offset(m_cbvDescriptorSize);
	}

	/*
	CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(DX::c_frameCount * c_alignedConstantBufferSize);
	DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantBuffer)));

	NAME_D3D12_OBJECT(m_constantBuffer);

	// Create constant buffer views to access the upload buffer.
	D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = m_constantBuffer->GetGPUVirtualAddress();
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
	m_cbvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int n = 0; n < DX::c_frameCount; n++)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		desc.BufferLocation = cbvGpuAddress;
		desc.SizeInBytes = c_alignedConstantBufferSize;
		d3dDevice->CreateConstantBufferView(&desc, cbvCpuHandle);

		cbvGpuAddress += desc.SizeInBytes;
		cbvCpuHandle.Offset(m_cbvDescriptorSize);
	}
	*/
}

// Renders one frame using the vertex and pixel shaders.
bool Sample3DSceneRenderer::Render(Model* model)
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return false;
	}

	DX::ThrowIfFailed(m_deviceResources->GetCommandAllocator()->Reset());

	// The command list can be reset anytime after ExecuteCommandList() is called.
	DX::ThrowIfFailed(m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get()));

	PIXBeginEvent(m_commandList.Get(), 0, L"Draw the cube");
	{
		// Set the graphics root signature and descriptor heaps to be used by this frame.
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() }; //Daniel: We only have one heap which is holding both our SRV and CB! (it's of that type!)
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		// Bind the current frame's constant buffer to the pipeline.
		//DANIEL: SECOND PARAM THERE HAS TO be 0!!! THEN FOR NEXT SRV IT HAS TO BE 1!! WTF IS THIS CURRFRAME STUFF??
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), 0, m_cbvDescriptorSize);
		//CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
		m_commandList->SetGraphicsRootDescriptorTable(0, gpuHandle); //This binds our CB

		//gpuHandle.Offset(m_cbvDescriptorSize);
		//gpuHandle.Offset(m_cbvDescriptorSize);
		//gpuHandle.Offset(m_cbvDescriptorSize);
		gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), 3, m_cbvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(1, gpuHandle); //Bind our tex

		//m_commandList->SetGraphicsRootShaderResourceView(321421, model->m_Textures[0].m_AlbedoTex->GetGPUVirtualAddress());
		//https://www.gamedev.net/forums/topic/710401-directx12-limitation-of-function-setcomputerootshaderresourceview/
		//m_commandList->SetGraphicsRootShaderResourceView(1, model->m_Textures[0].m_AlbedoTex->GetGPUVirtualAddress()); WTF DIDNT THIS WORK!!??
		{
			/*
			D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = m_constantBuffer->GetGPUVirtualAddress();
			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
			m_cbvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			for (int n = 0; n < DX::c_frameCount; n++)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
				desc.BufferLocation = cbvGpuAddress;
				desc.SizeInBytes = c_alignedConstantBufferSize;
				d3dDevice->CreateConstantBufferView(&desc, cbvCpuHandle);

				cbvGpuAddress += desc.SizeInBytes;
				cbvCpuHandle.Offset(m_cbvDescriptorSize);
			}
			*/
			//D3D12_GPU_VIRTUAL_ADDRESS srvGpuAddress = m_constantBuffer->GetGPUVirtualAddress() + c_alignedConstantBufferSize;

			//CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandleSrv(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), m_deviceResources->GetCurrentFrameIndex(), m_cbvDescriptorSize + m_deviceResources->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
			//m_commandList->SetGraphicsRootDescriptorTable(1, gpuHandle); //This binds our SRV.. TODO: It's wrong..
		}
		// Set the viewport and scissor rectangle.
		D3D12_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
		m_commandList->RSSetViewports(1, &viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		// Indicate this resource will be in use as a render target.
		CD3DX12_RESOURCE_BARRIER renderTargetResourceBarrier =
			CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &renderTargetResourceBarrier);

		// Record drawing commands.
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = m_deviceResources->GetRenderTargetView();
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = m_deviceResources->GetDepthStencilView();
		m_commandList->ClearRenderTargetView(renderTargetView, DirectX::Colors::CornflowerBlue, 0, nullptr);
		m_commandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		m_commandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);

		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		//m_commandList->IASetIndexBuffer(&m_indexBufferView);
		//m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0); //Daniel: I suspect this one! DrawInstancedIndex() will use the index buffer, use DrawInstanced instead

		int vertexBufferOffset = 0;
		int nrOfComponents = (int)model->GetNrOfVerticesList().size();
		vector<int> vertexComponentList = model->GetNrOfVerticesList();
		for(int i = 0; i < nrOfComponents; i++)
		{
			//int totalModelVertices = (UINT)model->GetNrOfVertices();
			int nrOfVerticesInComponent = vertexComponentList[i]*6; //The startVertex loc needs to be multiplied with 6 because each vertex has 6 floats (3 for pos, 3 for normal) ACTUALLY NO... not sure about why 6 is needed there...
			m_commandList->DrawInstanced((UINT)nrOfVerticesInComponent, 1, (UINT)vertexBufferOffset, 0); //third param = starvertex location
			vertexBufferOffset += nrOfVerticesInComponent;
		}

		//To draw the whole model in one go
		//m_commandList->DrawInstanced((UINT)model->GetNrOfVertices(), 1, 0, 0);

		// Indicate that the render target will now be used to present when the command list is done executing.
		CD3DX12_RESOURCE_BARRIER presentResourceBarrier =
			CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &presentResourceBarrier);
	}
	PIXEndEvent(m_commandList.Get());

	DX::ThrowIfFailed(m_commandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	return true;
}
