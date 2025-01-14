//***************************************************************************************
// BlendDemo.cpp by Frank Luna (C) 2011 All Rights Reserved.
//
// Demonstrates blending, HLSL clip(), and fogging.
//
// Controls:
//		Hold the left mouse button down and move the mouse to rotate.
//      Hold the right mouse button down to zoom in and out.
//
//      Press '1' - Lighting only render mode.
//      Press '2' - Texture render mode.
//      Press '3' - Fog render mode.
//
//***************************************************************************************

#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Vertex.h"
#include "RenderStates.h"
#include "Waves.h"
#include <DDSTextureLoader.h>
#include <d3dcompiler.h>

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

struct BlendDemoPerFrameStruct
{
	BlendDemoPerFrameStruct() { ZeroMemory(this, sizeof(this)); }

	DirectionalLight gDirLights[3];
	DirectX::XMFLOAT3 gEyePosW;
	float padding1;

	float  gFogStart;
	float  gFogRange;
	DirectX::XMFLOAT2 padding2;

	DirectX::XMFLOAT4 gFogColor;
};

struct BlendDemoPerObjectStruct
{
	BlendDemoPerObjectStruct() { ZeroMemory(this, sizeof(this)); }

	DirectX::XMMATRIX World;
	DirectX::XMMATRIX WorldInvTranspose;
	DirectX::XMMATRIX WorldViewProj;
	DirectX::XMMATRIX TexTransform;
	Material Material;
	int useTexture;
	int alphaClip;
	int fogEnabled;
};

class BlendApp : public D3DApp
{
public:
	BlendApp(HINSTANCE hInstance);
	~BlendApp();

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
	void BuildCrateGeometryBuffers();

private:
	ID3D11Buffer* mLandVB;
	ID3D11Buffer* mLandIB;

	ID3D11Buffer* mWavesVB;
	ID3D11Buffer* mWavesIB;

	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	ID3D11ShaderResourceView* mGrassMapSRV;
	ID3D11ShaderResourceView* mWavesMapSRV;
	ID3D11ShaderResourceView* mBoxMapSRV;

	BlendDemoPerObjectStruct perObjectStruct;
	BlendDemoPerFrameStruct perFrameStruct;
	ID3D11VertexShader* mVS;
	ID3D11PixelShader* mPS;
	ID3D11Buffer* perFrameBuffer;
	ID3D11Buffer* perObjectBuffer;
	ID3D11SamplerState* samplerState;
	ID3D10Blob* mVSBlob;

	ID3D11InputLayout* mInputLayout;


	Waves mWaves;

	DirectionalLight mDirLights[3];
	Material mLandMat;
	Material mWavesMat;
	Material mBoxMat;

	DirectX::XMFLOAT4X4 mGrassTexTransform;
	DirectX::XMFLOAT4X4 mWaterTexTransform;
	DirectX::XMFLOAT4X4 mLandWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;
	DirectX::XMFLOAT4X4 mBoxWorld;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	UINT mLandIndexCount;

	DirectX::XMFLOAT2 mWaterTexOffset;

	RenderOptions mRenderOptions;
	bool landUseTexture;
	bool landAlphaClip;
	bool landFogEnabled;
	bool boxUseTexture;
	bool boxAlphaClip;
	bool boxFogEnabled;

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

	BlendApp theApp(hInstance);
	
	if( !theApp.Init() )
		return 0;
	
	return theApp.Run();
}

BlendApp::BlendApp(HINSTANCE hInstance)
: D3DApp(hInstance), mLandVB(0), mLandIB(0), mWavesVB(0), mWavesIB(0), mBoxVB(0), mBoxIB(0), mGrassMapSRV(0), mWavesMapSRV(0), mBoxMapSRV(0),
  mWaterTexOffset(0.0f, 0.0f), mEyePosW(0.0f, 0.0f, 0.0f), mLandIndexCount(0), mRenderOptions(RenderOptions::TexturesAndFog),
  mTheta(1.3f*MathHelper::Pi), mPhi(0.4f*MathHelper::Pi), mRadius(80.0f)
{
	mMainWndCaption = L"Blend Demo";
	mEnable4xMsaa = false;

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mLandWorld, I);
	XMStoreFloat4x4(&mWavesWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	DirectX::XMMATRIX boxScale = DirectX::XMMatrixScaling(15.0f, 15.0f, 15.0f);
	DirectX::XMMATRIX boxOffset = DirectX::XMMatrixTranslation(8.0f, 5.0f, -15.0f);
	XMStoreFloat4x4(&mBoxWorld, boxScale*boxOffset);

	DirectX::XMMATRIX grassTexScale = DirectX::XMMatrixScaling(5.0f, 5.0f, 0.0f);
	XMStoreFloat4x4(&mGrassTexTransform, grassTexScale);

	mDirLights[0].Ambient  = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[0].Diffuse  = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Specular = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	mDirLights[1].Ambient  = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[1].Diffuse  = DirectX::XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	mDirLights[1].Specular = DirectX::XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	mDirLights[1].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	mDirLights[2].Ambient  = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Diffuse  = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[2].Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Direction = DirectX::XMFLOAT3(0.0f, -0.707f, -0.707f);

	mLandMat.Ambient  = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mLandMat.Diffuse  = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mLandMat.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	mWavesMat.Ambient  = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mWavesMat.Diffuse  = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	mWavesMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

	mBoxMat.Ambient  = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mBoxMat.Diffuse  = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
}

BlendApp::~BlendApp()
{
	md3dImmediateContext->ClearState();
	ReleaseCOM(mLandVB);
	ReleaseCOM(mLandIB);
	ReleaseCOM(mWavesVB);
	ReleaseCOM(mWavesIB);
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mGrassMapSRV);
	ReleaseCOM(mWavesMapSRV);
	ReleaseCOM(mBoxMapSRV);

	ReleaseCOM(mVS);
	ReleaseCOM(mPS);
	ReleaseCOM(perFrameBuffer);
	ReleaseCOM(perObjectBuffer);
	ReleaseCOM(mVSBlob);

	RenderStates::DestroyAll();
}

void BlendApp::CheckCompilationErrors(ID3D10Blob* compilationMsgs)
{
	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}
}

bool BlendApp::Init()
{
	if(!D3DApp::Init())
		return false;

	mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

	// Must init Effects first since InputLayouts depend on shader signatures.
	RenderStates::InitAll(md3dDevice);

	size_t maxsize = D3DX11_FROM_FILE;
	D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
	unsigned int bindFlags = D3D11_BIND_SHADER_RESOURCE;
	unsigned int cpuAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	unsigned int miscFlags = 0;
	DirectX::DX11::DDS_LOADER_FLAGS loadFlags = DirectX::DX11::DDS_LOADER_DEFAULT;

	HR(CreateDDSTextureFromFileEx(md3dDevice, L"Textures/grass.dds", maxsize, usage, bindFlags, cpuAccessFlags, miscFlags,
		loadFlags, nullptr, &mGrassMapSRV, 0));

	HR(CreateDDSTextureFromFileEx(md3dDevice, L"Textures/water2.dds", maxsize, usage, bindFlags, cpuAccessFlags, miscFlags,
		loadFlags, nullptr, &mWavesMapSRV, 0));

	HR(CreateDDSTextureFromFileEx(md3dDevice, L"Textures/WireFence.dds", maxsize, usage, bindFlags, cpuAccessFlags, miscFlags,
		loadFlags, nullptr, &mBoxMapSRV, 0));

	BuildLandGeometryBuffers();
	BuildWaveGeometryBuffers();
	BuildCrateGeometryBuffers();

	ID3D10Blob* compiledShader = 0;
	ID3D10Blob* compilationMsgs = 0;
	HR(D3DCompileFromFile(L"FX/Basic.fx", 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0,
		0, &mVSBlob, &compilationMsgs));

	CheckCompilationErrors(compilationMsgs);

	HR(md3dDevice->CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVS));

	HR(D3DCompileFromFile(L"FX/Basic.fx", 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0,
		0, &compiledShader, &compilationMsgs));

	CheckCompilationErrors(compilationMsgs);

	HR(md3dDevice->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, &mPS));
	ReleaseCOM(compiledShader);

	// Create a constant buffer for the shader constants
	D3D11_BUFFER_DESC perObjectConstantBufferDesc = { sizeof(perObjectStruct), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE };
	D3D11_BUFFER_DESC perFrameConstantBufferDesc = { sizeof(perFrameStruct), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE };
	HR(md3dDevice->CreateBuffer(&perObjectConstantBufferDesc, nullptr, &perObjectBuffer));
	HR(md3dDevice->CreateBuffer(&perFrameConstantBufferDesc, nullptr, &perFrameBuffer));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HR(md3dDevice->CreateInputLayout(vertexDesc, 3, mVSBlob->GetBufferPointer(),
		mVSBlob->GetBufferSize(), &mInputLayout));

	// Create the sampler
	D3D11_SAMPLER_DESC samplerDesc = {
		D3D11_FILTER_ANISOTROPIC,
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_WRAP,
		0,
		4,
		D3D11_COMPARISON_NEVER,
		{0.0f, 0.0f, 0.0f, 1.0f},
		0,
		D3D11_FLOAT32_MAX
	};

	HR(md3dDevice->CreateSamplerState(&samplerDesc, &samplerState));

	return true;
}

void BlendApp::OnResize()
{
	D3DApp::OnResize();

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BlendApp::UpdateScene(float dt)
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
	if( (mTimer.TotalTime() - t_base) >= 0.1f )
	{
		t_base += 0.1f;
 
		DWORD i = 5 + rand() % (mWaves.RowCount()-10);
		DWORD j = 5 + rand() % (mWaves.ColumnCount()-10);

		float r = MathHelper::RandF(0.5f, 1.0f);

		mWaves.Disturb(i, j, r);
	}

	mWaves.Update(dt);

	//
	// Update the wave vertex buffer with the new solution.
	//
	
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(md3dImmediateContext->Map(mWavesVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	Vertex::Basic32* v = reinterpret_cast<Vertex::Basic32*>(mappedData.pData);
	for(UINT i = 0; i < mWaves.VertexCount(); ++i)
	{
		v[i].Pos    = mWaves[i];
		v[i].Normal = mWaves.Normal(i);

		// Derive tex-coords in [0,1] from position.
		v[i].Tex.x  = 0.5f + mWaves[i].x / mWaves.Width();
		v[i].Tex.y  = 0.5f - mWaves[i].z / mWaves.Depth();
	}

	md3dImmediateContext->Unmap(mWavesVB, 0);

	//
	// Animate water texture coordinates.
	//

	// Tile water texture.
	DirectX::XMMATRIX wavesScale = DirectX::XMMatrixScaling(5.0f, 5.0f, 0.0f);

	// Translate texture over time.
	mWaterTexOffset.y += 0.05f*dt;
	mWaterTexOffset.x += 0.1f*dt;	
	DirectX::XMMATRIX wavesOffset = DirectX::XMMatrixTranslation(mWaterTexOffset.x, mWaterTexOffset.y, 0.0f);

	// Combine scale and translation.
	XMStoreFloat4x4(&mWaterTexTransform, wavesScale*wavesOffset);

	//
	// Switch the render mode based in key input.
	//
	if( GetAsyncKeyState('1') & 0x8000 )
		mRenderOptions = RenderOptions::Lighting; 

	if( GetAsyncKeyState('2') & 0x8000 )
		mRenderOptions = RenderOptions::Textures; 

	if( GetAsyncKeyState('3') & 0x8000 )
		mRenderOptions = RenderOptions::TexturesAndFog; 
}

void BlendApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
    md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 
	float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};

	UINT stride = sizeof(Vertex::Basic32);
    UINT offset = 0;
 
	DirectX::XMMATRIX view  = XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj  = XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX viewProj = view*proj;
	for (int i = 0; i < 3; i++)
	{
		perFrameStruct.gDirLights[i] = mDirLights[i];
	}

	perFrameStruct.gEyePosW.x = mEyePosW.x;
	perFrameStruct.gEyePosW.y = mEyePosW.y;
	perFrameStruct.gEyePosW.z = mEyePosW.z;
	perFrameStruct.gFogStart = 15.0f;
	perFrameStruct.gFogRange = 175.0f;
	perFrameStruct.gFogColor = Colors::Silver;

	// Set per frame constants.
	D3D11_MAPPED_SUBRESOURCE cbData;
	md3dImmediateContext->Map(perFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perFrameStruct, sizeof(perFrameStruct));
	md3dImmediateContext->Unmap(perFrameBuffer, 0);

	switch (mRenderOptions)
	{
	case RenderOptions::Lighting:
		boxUseTexture = false;
		boxAlphaClip = false;
		boxFogEnabled = false;
		landUseTexture = false;
		landAlphaClip = false;
		landFogEnabled = false;
		break;
	case RenderOptions::Textures:
		boxUseTexture = true;
		boxAlphaClip = true;
		boxFogEnabled = true;
		landUseTexture = true;
		landAlphaClip = false;
		landFogEnabled = false;
		break;
	case RenderOptions::TexturesAndFog:
		boxUseTexture = true;
		boxAlphaClip = true;
		boxFogEnabled = true;
		landUseTexture = true;
		landAlphaClip = false;
		landFogEnabled = true;
		break;
	}

	//
	// Draw the box with alpha clipping.
	// 
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	DirectX::XMMATRIX world = XMLoadFloat4x4(&mBoxWorld);
	DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	DirectX::XMMATRIX worldViewProj = world * view * proj;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.TexTransform = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
	perObjectStruct.Material = mBoxMat;
	perObjectStruct.useTexture = boxUseTexture;
	perObjectStruct.alphaClip = boxAlphaClip;
	perObjectStruct.fogEnabled = boxFogEnabled;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);
	md3dImmediateContext->PSSetShaderResources(0, 1, &mBoxMapSRV);
	md3dImmediateContext->PSSetSamplers(0, 1, &samplerState);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->RSSetState(RenderStates::NoCullRS);
	md3dImmediateContext->DrawIndexed(36, 0, 0);

	// Restore default render state.
	md3dImmediateContext->RSSetState(0);

	//
	// Draw the hills and water with texture and fog (no alpha clipping needed).
	//
	// Draw hills
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mLandVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mLandIB, DXGI_FORMAT_R32_UINT, 0);

	world = XMLoadFloat4x4(&mLandWorld);
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world * view * proj;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.TexTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mGrassTexTransform));
	perObjectStruct.Material = mLandMat;
	perObjectStruct.alphaClip = landAlphaClip;
	perObjectStruct.useTexture = landUseTexture;
	perObjectStruct.fogEnabled = landFogEnabled;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);
	md3dImmediateContext->PSSetShaderResources(0, 1, &mGrassMapSRV);
	md3dImmediateContext->PSSetSamplers(0, 1, &samplerState);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->DrawIndexed(mLandIndexCount, 0, 0);

	// Draw water
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mWavesVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mWavesIB, DXGI_FORMAT_R32_UINT, 0);

	world = XMLoadFloat4x4(&mWavesWorld);
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world * view * proj;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.TexTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mWaterTexTransform));
	perObjectStruct.Material = mWavesMat;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);
	md3dImmediateContext->PSSetShaderResources(0, 1, &mWavesMapSRV);
	md3dImmediateContext->PSSetSamplers(0, 1, &samplerState);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->OMSetBlendState(RenderStates::NoRedGreenBS, blendFactor, 0xffffffff);
	//md3dImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
	//md3dImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xfffffffe);
	md3dImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);

	// Restore default blend state
	md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);

	HR(mSwapChain->Present(0, 0));
}

void BlendApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BlendApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BlendApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		// Make each pixel correspond to 0.01 unit in the scene.
		float dx = 0.1f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.1f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 20.0f, 500.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

float BlendApp::GetHillHeight(float x, float z)const
{
	return 0.3f*( z*sinf(0.1f*x) + x*cosf(0.1f*z) );
}

DirectX::XMFLOAT3 BlendApp::GetHillNormal(float x, float z)const
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

void BlendApp::BuildLandGeometryBuffers()
{
	GeometryGenerator::MeshData grid;
 
	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	mLandIndexCount = grid.Indices.size();

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  
	//

	std::vector<Vertex::Basic32> vertices(grid.Vertices.size());
	for(UINT i = 0; i < grid.Vertices.size(); ++i)
	{
		DirectX::XMFLOAT3 p = grid.Vertices[i].Position;

		p.y = GetHillHeight(p.x, p.z);
		
		vertices[i].Pos    = p;
		vertices[i].Normal = GetHillNormal(p.x, p.z);
		vertices[i].Tex    = grid.Vertices[i].TexC;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * grid.Vertices.size();
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

void BlendApp::BuildWaveGeometryBuffers()
{
	// Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * mWaves.VertexCount();
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

void BlendApp::BuildCrateGeometryBuffers()
{
	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex::Basic32> vertices(box.Vertices.size());

	for(UINT i = 0; i < box.Vertices.size(); ++i)
	{
		vertices[i].Pos    = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].Tex    = box.Vertices[i].TexC;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::Basic32) * box.Vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * box.Indices.size();
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &box.Indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}