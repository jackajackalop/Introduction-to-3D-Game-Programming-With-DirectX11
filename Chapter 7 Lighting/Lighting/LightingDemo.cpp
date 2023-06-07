//***************************************************************************************
// LightingDemo.cpp by Frank Luna (C) 2011 All Rights Reserved.
//
// Demonstrates 3D lighting with directional, point, and spot lights.
//
// Controls:
//		Hold the left mouse button down and move the mouse to rotate.
//      Hold the right mouse button down to zoom in and out.
//
//***************************************************************************************

#include "d3dApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Waves.h"
#include <d3dcompiler.h>

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
};

class LightingApp : public D3DApp
{
public:
	LightingApp(HINSTANCE hInstance);
	~LightingApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene(); 

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void CheckCompilationErrors(ID3D10Blob* compilationMsgs);
	float GetHillHeight(float x, float z)const;
	DirectX::XMFLOAT3 GetHillNormal(float x, float z)const;
	void BuildLandGeometryBuffers();
	void BuildWaveGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

private:
	ID3D11Buffer* mLandVB;
	ID3D11Buffer* mLandIB;

	ID3D11Buffer* mWavesVB;
	ID3D11Buffer* mWavesIB;

	Waves mWaves;
	DirectionalLight mDirLight;
	PointLight mPointLight;
	SpotLight mSpotLight;
	Material mLandMat;
	Material mWavesMat;

	ID3D11VertexShader* mVS;
	ID3D11PixelShader* mPS;
	PerObjectStruct perObjectStruct;
	PerFrameStruct perFrameStruct;
	ID3D11Buffer* perFrameBuffer;
	ID3D11Buffer* perObjectBuffer;
	ID3D10Blob* mVSBlob;

	ID3D11InputLayout* mInputLayout;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mLandWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	UINT mLandIndexCount;

	DirectX::XMFLOAT3 mEyePosW;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	LightingApp theApp(hInstance);
	
	if( !theApp.Init() )
		return 0;
	
	return theApp.Run();
}
 

LightingApp::LightingApp(HINSTANCE hInstance)
: D3DApp(hInstance), mLandVB(0), mLandIB(0), mWavesVB(0), mWavesIB(0), 
  mInputLayout(0), mEyePosW(0.0f, 0.0f, 0.0f), mTheta(1.5f*MathHelper::Pi), mPhi(0.1f*MathHelper::Pi), mRadius(80.0f)
{
	mMainWndCaption = L"Lighting Demo";
	
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mLandWorld, I);
	XMStoreFloat4x4(&mWavesWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	DirectX::XMMATRIX wavesOffset = DirectX::XMMatrixTranslation(0.0f, -3.0f, 0.0f);
	XMStoreFloat4x4(&mWavesWorld, wavesOffset);

	// Directional light.
	mDirLight.Ambient  = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLight.Diffuse  = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLight.Specular = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLight.Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);
 
	// Point light--position is changed every frame to animate in UpdateScene function.
	mPointLight.Ambient  = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mPointLight.Diffuse  = DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	mPointLight.Specular = DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	mPointLight.Att      = DirectX::XMFLOAT3(0.0f, 0.1f, 0.0f);
	mPointLight.Range    = 25.0f;

	// Spot light--position and direction changed every frame to animate in UpdateScene function.
	mSpotLight.Ambient  = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mSpotLight.Diffuse  = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	mSpotLight.Specular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mSpotLight.Att      = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
	mSpotLight.Spot     = 96.0f;
	mSpotLight.Range    = 10000.0f;

	mLandMat.Ambient  = DirectX::XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	mLandMat.Diffuse  = DirectX::XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	mLandMat.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	mWavesMat.Ambient  = DirectX::XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
	mWavesMat.Diffuse  = DirectX::XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
	mWavesMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 96.0f);
}

LightingApp::~LightingApp()
{
	ReleaseCOM(mLandVB);
	ReleaseCOM(mLandIB);
	ReleaseCOM(mWavesVB);
	ReleaseCOM(mWavesIB);

	ReleaseCOM(mInputLayout);
	ReleaseCOM(mVS);
	ReleaseCOM(mPS);
	ReleaseCOM(perFrameBuffer);
	ReleaseCOM(perObjectBuffer);
	ReleaseCOM(mVSBlob);
}

bool LightingApp::Init()
{
	if(!D3DApp::Init())
		return false;

	mWaves.Init(160, 160, 1.0f, 0.03f, 3.25f, 0.4f);

	BuildLandGeometryBuffers();
	BuildWaveGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	return true;
}

void LightingApp::OnResize()
{
	D3DApp::OnResize();

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void LightingApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius*sinf(mPhi)*cosf(mTheta);
	float z = mRadius*sinf(mPhi)*sinf(mTheta);
	float y = mRadius*cosf(mPhi);

	mEyePosW = DirectX::XMFLOAT3(x, y, z);

	// Build the view matrix.
	DirectX::XMVECTOR pos    = DirectX::XMVectorSet(x, y, z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up     = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);

	//
	// Every quarter second, generate a random wave.
	//
	static float t_base = 0.0f;
	if( (mTimer.TotalTime() - t_base) >= 0.25f )
	{
		t_base += 0.25f;
 
		DWORD i = 5 + rand() % (mWaves.RowCount()-10);
		DWORD j = 5 + rand() % (mWaves.ColumnCount()-10);

		float r = MathHelper::RandF(1.0f, 2.0f);

		mWaves.Disturb(i, j, r);
	}

	mWaves.Update(dt);

	//
	// Update the wave vertex buffer with the new solution.
	//
	
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(md3dImmediateContext->Map(mWavesVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	Vertex* v = reinterpret_cast<Vertex*>(mappedData.pData);
	for(UINT i = 0; i < mWaves.VertexCount(); ++i)
	{
		v[i].Pos    = mWaves[i];
		v[i].Normal = mWaves.Normal(i);
	}

	md3dImmediateContext->Unmap(mWavesVB, 0);

	//
	// Animate the lights.
	//

	// Circle light over the land surface.
	mPointLight.Position.x = 70.0f*cosf( 0.2f*mTimer.TotalTime() );
	mPointLight.Position.z = 70.0f*sinf( 0.2f*mTimer.TotalTime() );
	mPointLight.Position.y = MathHelper::Max(GetHillHeight(mPointLight.Position.x, 
		mPointLight.Position.z), -3.0f) + 10.0f;


	// The spotlight takes on the camera position and is aimed in the
	// same direction the camera is looking.  In this way, it looks
	// like we are holding a flashlight.
	mSpotLight.Position = mEyePosW;
	XMStoreFloat3(&mSpotLight.Direction, DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, pos)));
}

void LightingApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	DirectX::XMMATRIX view = XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj = XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX viewProj = view * proj;
	perFrameStruct.DirLight = mDirLight;
	perFrameStruct.PointLight = mPointLight;
	perFrameStruct.SpotLight = mSpotLight;
	perFrameStruct.EyePosW.x = mEyePosW.x;
	perFrameStruct.EyePosW.y = mEyePosW.y;
	perFrameStruct.EyePosW.z = mEyePosW.z;

	// Set per frame constants.
	D3D11_MAPPED_SUBRESOURCE cbData;
	md3dImmediateContext->Map(perFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perFrameStruct, sizeof(perFrameStruct));
	md3dImmediateContext->Unmap(perFrameBuffer, 0);

	//
	// Draw the hills.
	//
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mLandVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mLandIB, DXGI_FORMAT_R32_UINT, 0);

	// Set per object constants.
	DirectX::XMMATRIX world = XMLoadFloat4x4(&mLandWorld);
	world = DirectX::XMMatrixTranspose(world);
	DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	worldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	DirectX::XMMATRIX worldViewProj = world * view * proj;
	worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.World = world;
	perObjectStruct.WorldInvTranspose = worldInvTranspose;
	perObjectStruct.WorldViewProj = worldViewProj;
	perObjectStruct.Material = mLandMat;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);

	md3dImmediateContext->DrawIndexed(mLandIndexCount, 0, 0);

	//
	// Draw the waves.
	//
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mWavesVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mWavesIB, DXGI_FORMAT_R32_UINT, 0);

	// Set per object constants.
	world = XMLoadFloat4x4(&mWavesWorld);
	world = DirectX::XMMatrixTranspose(world);
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	worldViewProj = world * view * proj;
	worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.World = world;
	perObjectStruct.WorldInvTranspose = worldInvTranspose;
	perObjectStruct.WorldViewProj = worldViewProj;
	perObjectStruct.Material = mWavesMat;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);
	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);

	HR(mSwapChain->Present(0, 0));
}

void LightingApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void LightingApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void LightingApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if( (btnState & MK_LBUTTON) != 0 )
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi   += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi-0.1f);
	}
	else if( (btnState & MK_RBUTTON) != 0 )
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.2f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 50.0f, 500.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

float LightingApp::GetHillHeight(float x, float z)const
{
	return 0.3f*( z*sinf(0.1f*x) + x*cosf(0.1f*z) );
}

DirectX::XMFLOAT3 LightingApp::GetHillNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	DirectX::XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));
	
	DirectX::XMVECTOR unitNormal = DirectX::XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void LightingApp::BuildLandGeometryBuffers()
{
	GeometryGenerator::MeshData grid;
 
	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	mLandIndexCount = grid.Indices.size();

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for(size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		DirectX::XMFLOAT3 p = grid.Vertices[i].Position;

		p.y = GetHillHeight(p.x, p.z);
		
		vertices[i].Pos    = p;
		vertices[i].Normal = GetHillNormal(p.x, p.z);
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * grid.Vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mLandVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mLandIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.Indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mLandIB));
}

void LightingApp::BuildWaveGeometryBuffers()
{
	// Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex) * mWaves.VertexCount();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbd.MiscFlags = 0;
    HR(md3dDevice->CreateBuffer(&vbd, 0, &mWavesVB));


	// Create the index buffer.  The index buffer is fixed, so we only 
	// need to create and set once.

	std::vector<UINT> indices(3*mWaves.TriangleCount()); // 3 indices per face

	// Iterate over each quad.
	UINT m = mWaves.RowCount();
	UINT n = mWaves.ColumnCount();
	int k = 0;
	for(UINT i = 0; i < m-1; ++i)
	{
		for(DWORD j = 0; j < n-1; ++j)
		{
			indices[k]   = i*n+j;
			indices[k+1] = i*n+j+1;
			indices[k+2] = (i+1)*n+j;

			indices[k+3] = (i+1)*n+j;
			indices[k+4] = i*n+j+1;
			indices[k+5] = (i+1)*n+j+1;

			k += 6; // next quad
		}
	}

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * indices.size();
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mWavesIB));
}

void LightingApp::CheckCompilationErrors(ID3D10Blob* compilationMsgs)
{
	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}
}

void LightingApp::BuildFX()
{
	ID3D10Blob* compiledShader = 0;
	ID3D10Blob* compilationMsgs = 0;

	HR(D3DCompileFromFile(L"FX/Lighting.fx", 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0,
		0, &mVSBlob, &compilationMsgs));

	CheckCompilationErrors(compilationMsgs);

	HR(md3dDevice->CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVS));

	HR(D3DCompileFromFile(L"FX/Lighting.fx", 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0,
		0, &compiledShader, &compilationMsgs));

	CheckCompilationErrors(compilationMsgs);

	HR(md3dDevice->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, &mPS));
	ReleaseCOM(compiledShader);

	// Create a constant buffer for the shader constants
	D3D11_BUFFER_DESC perObjectConstantBufferDesc = { sizeof(perObjectStruct), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE };
	D3D11_BUFFER_DESC perFrameConstantBufferDesc = { sizeof(perFrameStruct), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE };
	HR(md3dDevice->CreateBuffer(&perObjectConstantBufferDesc, nullptr, &perObjectBuffer));
	HR(md3dDevice->CreateBuffer(&perFrameConstantBufferDesc, nullptr, &perFrameBuffer));
}

void LightingApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HR(md3dDevice->CreateInputLayout(vertexDesc, 2, mVSBlob->GetBufferPointer(),
		mVSBlob->GetBufferSize(), &mInputLayout));
}