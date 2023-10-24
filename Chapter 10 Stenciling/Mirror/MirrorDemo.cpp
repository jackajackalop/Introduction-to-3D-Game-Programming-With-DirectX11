//***************************************************************************************
// MirrorDemo.cpp by Frank Luna (C) 2011 All Rights Reserved.
//
// Demonstrates using the stencil buffer to mask out areas from being drawn to, 
// and to prevent "double blending."
//
// Controls:
//		Hold the left mouse button down and move the mouse to rotate.
//      Hold the right mouse button down to zoom in and out.
//
//      Press '1' - Lighting only render mode.
//      Press '2' - Texture render mode.
//      Press '3' - Fog render mode.
//
//		Move the skull left/right/up/down with 'A'/'D'/'W'/'S' keys.
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

struct MirrorDemoPerFrameStruct
{
	MirrorDemoPerFrameStruct() { ZeroMemory(this, sizeof(this)); }

	DirectionalLight gDirLights[3];
	DirectX::XMFLOAT4 gEyePosW;

	float  gFogStart;
	float  gFogRange;
	DirectX::XMFLOAT2 padding;

	DirectX::XMFLOAT4 gFogColor;
};

struct MirrorDemoPerObjectStruct
{
	MirrorDemoPerObjectStruct() { ZeroMemory(this, sizeof(this)); }

	DirectX::XMMATRIX World;
	DirectX::XMMATRIX WorldInvTranspose;
	DirectX::XMMATRIX WorldViewProj;
	DirectX::XMMATRIX TexTransform;
	Material Material;
};

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

class MirrorApp : public D3DApp
{
public:
	MirrorApp(HINSTANCE hInstance);
	~MirrorApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene(); 

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void CheckCompilationErrors(ID3D10Blob* compilationMsgs);
	void BuildRoomGeometryBuffers();
	void BuildSkullGeometryBuffers();

private:
	ID3D11Buffer* mRoomVB;

	ID3D11Buffer* mSkullVB;
	ID3D11Buffer* mSkullIB;

	MirrorDemoPerObjectStruct perObjectStruct;
	MirrorDemoPerFrameStruct perFrameStruct;
	ID3D11VertexShader* mVS;
	ID3D11PixelShader* mPS;
	ID3D11Buffer* perFrameBuffer;
	ID3D11Buffer* perObjectBuffer;
	ID3D11SamplerState* samplerState;
	ID3D10Blob* mVSBlob;

	ID3D11InputLayout* mInputLayout;

	ID3D11ShaderResourceView* mFloorDiffuseMapSRV;
	ID3D11ShaderResourceView* mWallDiffuseMapSRV;
	ID3D11ShaderResourceView* mMirrorDiffuseMapSRV;

	DirectionalLight mDirLights[3];
	Material mRoomMat;
	Material mSkullMat;
	Material mMirrorMat;
	Material mShadowMat;

	DirectX::XMFLOAT4X4 mRoomWorld;
	DirectX::XMFLOAT4X4 mSkullWorld;

	UINT mSkullIndexCount;
	DirectX::XMFLOAT3 mSkullTranslation;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	RenderOptions mRenderOptions;
	
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

	MirrorApp theApp(hInstance);
	
	if( !theApp.Init() )
		return 0;
	
	return theApp.Run();
}

MirrorApp::MirrorApp(HINSTANCE hInstance)
: D3DApp(hInstance), mRoomVB(0), mSkullVB(0), mSkullIB(0), mSkullIndexCount(0), mSkullTranslation(0.0f, 1.0f, -5.0f),
  mFloorDiffuseMapSRV(0), mWallDiffuseMapSRV(0), mMirrorDiffuseMapSRV(0), 
  mEyePosW(0.0f, 0.0f, 0.0f), mRenderOptions(RenderOptions::Textures),
  mTheta(1.24f*MathHelper::Pi), mPhi(0.42f*MathHelper::Pi), mRadius(12.0f)
{
	mMainWndCaption = L"Mirror Demo";
	mEnable4xMsaa = false;

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mRoomWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

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

	mRoomMat.Ambient  = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mRoomMat.Diffuse  = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mRoomMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	mSkullMat.Ambient  = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mSkullMat.Diffuse  = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mSkullMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	// Reflected material is transparent so it blends into mirror.
	mMirrorMat.Ambient  = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mMirrorMat.Diffuse  = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	mMirrorMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	mShadowMat.Ambient  = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mShadowMat.Diffuse  = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	mShadowMat.Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);
}

MirrorApp::~MirrorApp()
{
	md3dImmediateContext->ClearState();
	ReleaseCOM(mRoomVB);
	ReleaseCOM(mSkullVB);
	ReleaseCOM(mSkullIB);
	ReleaseCOM(mFloorDiffuseMapSRV);
	ReleaseCOM(mWallDiffuseMapSRV);
	ReleaseCOM(mMirrorDiffuseMapSRV);

	ReleaseCOM(mVS);
	ReleaseCOM(mPS);
	ReleaseCOM(perFrameBuffer);
	ReleaseCOM(perObjectBuffer);
	ReleaseCOM(mVSBlob);
	RenderStates::DestroyAll();
}

void MirrorApp::CheckCompilationErrors(ID3D10Blob* compilationMsgs)
{
	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}
}

bool MirrorApp::Init()
{
	if(!D3DApp::Init())
		return false;

	// Must init Effects first since InputLayouts depend on shader signatures.
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

	RenderStates::InitAll(md3dDevice);

	size_t maxsize = D3DX11_FROM_FILE;
	D3D11_USAGE usage = D3D11_USAGE_STAGING;
	unsigned int bindFlags = 0;
	unsigned int cpuAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	unsigned int miscFlags = 0;
	DirectX::DX11::DDS_LOADER_FLAGS loadFlags = DirectX::DX11::DDS_LOADER_DEFAULT;

	HR(CreateDDSTextureFromFileEx(md3dDevice, L"Textures/checkboard.dds", maxsize, usage, bindFlags, cpuAccessFlags, miscFlags,
		loadFlags, nullptr, &mFloorDiffuseMapSRV, 0));

	HR(CreateDDSTextureFromFileEx(md3dDevice, L"Textures/brick01.dds", maxsize, usage, bindFlags, cpuAccessFlags, miscFlags,
		loadFlags, nullptr, &mWallDiffuseMapSRV, 0));

	HR(CreateDDSTextureFromFileEx(md3dDevice, L"Textures/ice.dds", maxsize, usage, bindFlags, cpuAccessFlags, miscFlags,
		loadFlags, nullptr, &mMirrorDiffuseMapSRV, 0));
 
	BuildRoomGeometryBuffers();
	BuildSkullGeometryBuffers();

	return true;
}

void MirrorApp::OnResize()
{
	D3DApp::OnResize();

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void MirrorApp::UpdateScene(float dt)
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
	// Switch the render mode based in key input.
	//
	if( GetAsyncKeyState('1') & 0x8000 )
		mRenderOptions = RenderOptions::Lighting; 

	if( GetAsyncKeyState('2') & 0x8000 )
		mRenderOptions = RenderOptions::Textures; 

	if( GetAsyncKeyState('3') & 0x8000 )
		mRenderOptions = RenderOptions::TexturesAndFog; 


	//
	// Allow user to move box.
	//
	if( GetAsyncKeyState('A') & 0x8000 )
		mSkullTranslation.x -= 1.0f*dt;

	if( GetAsyncKeyState('D') & 0x8000 )
		mSkullTranslation.x += 1.0f*dt;

	if( GetAsyncKeyState('W') & 0x8000 )
		mSkullTranslation.y += 1.0f*dt;

	if( GetAsyncKeyState('S') & 0x8000 )
		mSkullTranslation.y -= 1.0f*dt;

	// Don't let user move below ground plane.
	mSkullTranslation.y = MathHelper::Max(mSkullTranslation.y, 0.0f);

	// Update the new world matrix.
	DirectX::XMMATRIX skullRotate = DirectX::XMMatrixRotationY(0.5f*MathHelper::Pi);
	DirectX::XMMATRIX skullScale = DirectX::XMMatrixScaling(0.45f, 0.45f, 0.45f);
	DirectX::XMMATRIX skullOffset = DirectX::XMMatrixTranslation(mSkullTranslation.x, mSkullTranslation.y, mSkullTranslation.z);
	XMStoreFloat4x4(&mSkullWorld, skullRotate*skullScale*skullOffset);
}

void MirrorApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
    md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 
	float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};

	UINT stride = sizeof(Vertex::Basic32);
    UINT offset = 0;
 
	DirectX::XMMATRIX view  = XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj  = XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX viewProj = view*proj;

	// Set per frame constants.
	for (int i = 0; i < 3; i++)
	{
		perFrameStruct.gDirLights[i] = mDirLights[i];
	}
	perFrameStruct.gEyePosW.x = mEyePosW.x;
	perFrameStruct.gEyePosW.y = mEyePosW.y;
	perFrameStruct.gEyePosW.z = mEyePosW.z;
	perFrameStruct.gFogColor = Colors::Black;
	perFrameStruct.gFogStart = 2.0f;
	perFrameStruct.gFogRange = 40.0f;
	D3D11_MAPPED_SUBRESOURCE cbData;
	md3dImmediateContext->Map(perFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perFrameStruct, sizeof(perFrameStruct));
	md3dImmediateContext->Unmap(perFrameBuffer, 0);
 
	//
	// Draw the floor and walls to the back buffer as normal.
	//

	md3dImmediateContext->IASetVertexBuffers(0, 1, &mRoomVB, &stride, &offset);

	// Draw the box.
	DirectX::XMMATRIX world = XMLoadFloat4x4(&mRoomWorld);
	DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	DirectX::XMMATRIX worldViewProj = world * view * proj;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.TexTransform = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
	perObjectStruct.Material = mRoomMat;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetSamplers(0, 1, &samplerState);

	// Floor
	md3dImmediateContext->PSSetShaderResources(0, 1, &mFloorDiffuseMapSRV);
	md3dImmediateContext->Draw(6, 0);

	// Wall
	md3dImmediateContext->PSSetShaderResources(0, 1, &mWallDiffuseMapSRV);
	md3dImmediateContext->Draw(18, 6);

	//
	// Draw the skull to the back buffer as normal.
	//
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

	world = XMLoadFloat4x4(&mSkullWorld);
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world * view * proj;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.Material = mSkullMat;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);

	//
	// Draw the mirror to stencil buffer only.
	//
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mRoomVB, &stride, &offset);

	world = XMLoadFloat4x4(&mRoomWorld);
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world * view * proj;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.Material = mSkullMat;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	// Do not write to render target.
	md3dImmediateContext->OMSetBlendState(RenderStates::NoRenderTargetWritesBS, blendFactor, 0xffffffff);

	// Render visible mirror pixels to stencil buffer.
	// Do not write mirror depth to depth buffer at this point, otherwise it will occlude the reflection.
	md3dImmediateContext->OMSetDepthStencilState(RenderStates::MarkMirrorDSS, 1);

	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->Draw(6, 24);

	// Restore states.
	md3dImmediateContext->OMSetDepthStencilState(0, 0);
	md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);

	//
	// Draw the skull reflection.
	//
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

	DirectX::XMVECTOR mirrorPlane = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	DirectX::XMMATRIX R = DirectX::XMMatrixReflect(mirrorPlane);
	world = XMLoadFloat4x4(&mSkullWorld) * R;
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world * view * proj;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.Material = mSkullMat;

	// Cache the old light directions, and reflect the light directions.
	DirectX::XMFLOAT3 oldLightDirections[3];
	for (int i = 0; i < 3; ++i)
	{
		oldLightDirections[i] = mDirLights[i].Direction;

		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mDirLights[i].Direction);
		DirectX::XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
		DirectX::XMStoreFloat3(&mDirLights[i].Direction, reflectedLightDir);
		perFrameStruct.gDirLights[i] = mDirLights[i];
		mDirLights[i].Direction = oldLightDirections[i];
	}

	md3dImmediateContext->Map(perFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perFrameStruct, sizeof(perFrameStruct));
	md3dImmediateContext->Unmap(perFrameBuffer, 0);

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);


	md3dImmediateContext->VSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);

	// Cull clockwise triangles for reflection.
	md3dImmediateContext->RSSetState(RenderStates::CullClockwiseRS);

	// Only draw reflection into visible mirror pixels as marked by the stencil buffer. 
	md3dImmediateContext->OMSetDepthStencilState(RenderStates::DrawReflectionDSS, 1);
	md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);

	// Restore default states.
	md3dImmediateContext->RSSetState(0);
	md3dImmediateContext->OMSetDepthStencilState(0, 0);

	//
	// Draw the mirror to the back buffer as usual but with transparency
	// blending so the reflection shows through.
	// 
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mRoomVB, &stride, &offset);

	world = XMLoadFloat4x4(&mRoomWorld);
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world * view * proj;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.Material = mMirrorMat;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);

	md3dImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
	md3dImmediateContext->PSSetShaderResources(0, 1, &mMirrorDiffuseMapSRV);
	md3dImmediateContext->Draw(6, 24);

	//
	// Draw the skull shadow.
	//
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

	DirectX::XMVECTOR shadowPlane = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
	DirectX::XMVECTOR toMainLight = DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&mDirLights[0].Direction));
	DirectX::XMMATRIX S = DirectX::XMMatrixShadow(shadowPlane, toMainLight);
	DirectX::XMMATRIX shadowOffsetY = DirectX::XMMatrixTranslation(0.0f, 0.001f, 0.0f);

	world = XMLoadFloat4x4(&mSkullWorld) * S * shadowOffsetY;
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world * view * proj;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.Material = mShadowMat;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->OMSetDepthStencilState(RenderStates::NoDoubleBlendDSS, 0);
	md3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);

	// Restore default states.
	md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
	md3dImmediateContext->OMSetDepthStencilState(0, 0);

	HR(mSwapChain->Present(0, 0));
}

void MirrorApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void MirrorApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void MirrorApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		float dx = 0.01f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.01f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 50.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void MirrorApp::BuildRoomGeometryBuffers()
{
	// Create and specify geometry.  For this sample we draw a floor
	// and a wall with a mirror on it.  We put the floor, wall, and
	// mirror geometry in one vertex buffer.
	//
	//   |--------------|
	//   |              |
    //   |----|----|----|
    //   |Wall|Mirr|Wall|
	//   |    | or |    |
    //   /--------------/
    //  /   Floor      /
	// /--------------/

 
	Vertex::Basic32 v[30];

	// Floor: Observe we tile texture coordinates.
	v[0] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[1] = Vertex::Basic32(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex::Basic32( 7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	
	v[3] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[4] = Vertex::Basic32( 7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	v[5] = Vertex::Basic32( 7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f);

	// Wall: Observe we tile texture coordinates, and that we
	// leave a gap in the middle for the mirror.
	v[6]  = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[7]  = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[8]  = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);
	
	v[9]  = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[10] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);
	v[11] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f);

	v[12] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[13] = Vertex::Basic32(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[14] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	
	v[15] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[16] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	v[17] = Vertex::Basic32(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f);

	v[18] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[19] = Vertex::Basic32(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[20] = Vertex::Basic32( 7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);
	
	v[21] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[22] = Vertex::Basic32( 7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);
	v[23] = Vertex::Basic32( 7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f);

	// Mirror
	v[24] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[25] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[26] = Vertex::Basic32( 2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	
	v[27] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[28] = Vertex::Basic32( 2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[29] = Vertex::Basic32( 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::Basic32) * 30;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = v;
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mRoomVB));
}

void MirrorApp::BuildSkullGeometryBuffers()
{
	std::ifstream fin("Models/skull.txt");
	
	if(!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;
	
	std::vector<Vertex::Basic32> vertices(vcount);
	for(UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	mSkullIndexCount = 3*tcount;
	std::vector<UINT> indices(mSkullIndexCount);
	for(UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i*3+0] >> indices[i*3+1] >> indices[i*3+2];
	}

	fin.close();

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * vcount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mSkullIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
}