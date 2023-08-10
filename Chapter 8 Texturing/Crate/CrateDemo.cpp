//***************************************************************************************
// CrateDemo.cpp by Frank Luna (C) 2011 All Rights Reserved.
//
// Demonstrates texturing a box.
//
// Controls:
//		Hold the left mouse button down and move the mouse to rotate.
//      Hold the right mouse button down to zoom in and out.
//
//***************************************************************************************

#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Vertex.h"
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include <d3dcompiler.h>

struct CrateDemoPerFrameStruct
{
	CrateDemoPerFrameStruct() { ZeroMemory(this, sizeof(this)); }

	DirectionalLight gDirLights[3];
	DirectX::XMFLOAT4 gEyePosW;

	float  gFogStart;
	float  gFogRange;
	DirectX::XMFLOAT2 padding;

	DirectX::XMFLOAT4 gFogColor;
};

struct CrateDemoPerObjectStruct
{
	CrateDemoPerObjectStruct() { ZeroMemory(this, sizeof(this)); }

	DirectX::XMMATRIX World;
	DirectX::XMMATRIX WorldInvTranspose;
	DirectX::XMMATRIX WorldViewProj;
	DirectX::XMMATRIX TexTransform;
	Material Material;
};

class CrateApp : public D3DApp
{
public:
	CrateApp(HINSTANCE hInstance);
	~CrateApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene(); 

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void CheckCompilationErrors(ID3D10Blob* compilationMsgs);
	void BuildGeometryBuffers();

private:
	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	CrateDemoPerObjectStruct perObjectStruct;
	CrateDemoPerFrameStruct perFrameStruct;
	ID3D11VertexShader* mVS;
	ID3D11PixelShader* mPS;
	ID3D11Buffer* perFrameBuffer;
	ID3D11Buffer* perObjectBuffer;
	ID3D11SamplerState* samplerState;
	ID3D10Blob* mVSBlob;

	ID3D11InputLayout* mInputLayout;
	
	ID3D11ShaderResourceView* mFlareSRV;
	ID3D11ShaderResourceView* mFlareAlphaSRV;
	ID3D11ShaderResourceView* mFireAnimSRV[120];

	DirectionalLight mDirLights[3];
	Material mBoxMat;

	DirectX::XMFLOAT4X4 mTexTransform;
	DirectX::XMFLOAT4X4 mBoxWorld;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	float rotationDegrees = 0.0f;
	int frame = 0;

	int mBoxVertexOffset;
	UINT mBoxIndexOffset;
	UINT mBoxIndexCount;

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

	CrateApp theApp(hInstance);
	
	if( !theApp.Init() )
		return 0;
	
	return theApp.Run();
}
 

CrateApp::CrateApp(HINSTANCE hInstance)
: D3DApp(hInstance), mBoxVB(0), mBoxIB(0), mFlareSRV(0), mFlareAlphaSRV(0), mEyePosW(0.0f, 0.0f, 0.0f),
  mTheta(1.3f*MathHelper::Pi), mPhi(0.4f*MathHelper::Pi), mRadius(2.5f), rotationDegrees(0)
{
	mMainWndCaption = L"Crate Demo";
	
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mBoxWorld, I);
	XMStoreFloat4x4(&mTexTransform, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	mDirLights[0].Ambient  = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mDirLights[0].Diffuse  = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mDirLights[0].Specular = DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
	mDirLights[0].Direction = DirectX::XMFLOAT3(0.707f, -0.707f, 0.0f);
 
	mDirLights[1].Ambient  = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[1].Diffuse  = DirectX::XMFLOAT4(1.4f, 1.4f, 1.4f, 1.0f);
	mDirLights[1].Specular = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 16.0f);
	mDirLights[1].Direction = DirectX::XMFLOAT3(-0.707f, 0.0f, 0.707f);

	mBoxMat.Ambient  = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mBoxMat.Diffuse  = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
}

CrateApp::~CrateApp()
{
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mFlareSRV);
	ReleaseCOM(mFlareAlphaSRV);
	ReleaseCOM(mVS);
	ReleaseCOM(mPS);
	ReleaseCOM(perFrameBuffer);
	ReleaseCOM(perObjectBuffer);
	ReleaseCOM(mVSBlob);

	InputLayouts::DestroyAll();
}

void CrateApp::CheckCompilationErrors(ID3D10Blob* compilationMsgs)
{
	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}
}

bool CrateApp::Init()
{
	if(!D3DApp::Init())
		return false;


	size_t maxsize = D3DX11_FROM_FILE;
	D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
	unsigned int bindFlags = D3D11_BIND_SHADER_RESOURCE;
	unsigned int cpuAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	unsigned int miscFlags = 0;
	DirectX::DX11::DDS_LOADER_FLAGS loadFlags = DirectX::DX11::DDS_LOADER_DEFAULT;

	HR(CreateDDSTextureFromFileEx(md3dDevice, L"Textures/flare.dds", maxsize, usage, bindFlags, cpuAccessFlags, miscFlags,
		loadFlags, nullptr, &mFlareSRV, 0));

	HR(CreateDDSTextureFromFileEx(md3dDevice, L"Textures/flarealpha.dds", maxsize, usage, bindFlags, cpuAccessFlags, miscFlags,
		loadFlags, nullptr, &mFlareAlphaSRV, 0));

	for (int i = 1; i <= 120; i++)
	{
		std::string str = std::to_string(i);
		int precision = 3 - min(3, str.size());
		str.insert(0, precision, '0');
		std::string filename = "FireAnim/Fire" + str + ".bmp";
		std::wstring widestr = std::wstring(filename.begin(), filename.end());
		HR(DirectX::CreateWICTextureFromFileEx(md3dDevice, widestr.c_str(), maxsize, usage, bindFlags, cpuAccessFlags, miscFlags,
			DirectX::WIC_LOADER_DEFAULT, nullptr, &mFireAnimSRV[i-1]));
	}

	BuildGeometryBuffers();

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
		// D3D11_FILTER_MIN_MAG_MIP_POINT,
		// D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		D3D11_FILTER_ANISOTROPIC, 
		/*D3D11_TEXTURE_ADDRESS_MIRROR,
		D3D11_TEXTURE_ADDRESS_MIRROR,
		D3D11_TEXTURE_ADDRESS_MIRROR,*/
		/*D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,*/
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		/*D3D11_TEXTURE_ADDRESS_WRAP, 
		D3D11_TEXTURE_ADDRESS_WRAP, 
		D3D11_TEXTURE_ADDRESS_WRAP,*/
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

void CrateApp::OnResize()
{
	D3DApp::OnResize();

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void CrateApp::UpdateScene(float dt)
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

	// rotationDegrees += 0.001f;
	frame = (frame + 1) % 1200;
}

void CrateApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
    md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 
	UINT stride = sizeof(Vertex::Basic32);
    UINT offset = 0;
 
	DirectX::XMMATRIX view  = XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj  = XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX viewProj = view*proj;

	for (int i = 0; i < 2; i++)
	{
		perFrameStruct.gDirLights[i] = mDirLights[i];
	}

	perFrameStruct.gEyePosW.x = mEyePosW.x;
	perFrameStruct.gEyePosW.y = mEyePosW.y;
	perFrameStruct.gEyePosW.z = mEyePosW.z;

	// Set per frame constants.
	D3D11_MAPPED_SUBRESOURCE cbData;
	md3dImmediateContext->Map(perFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perFrameStruct, sizeof(perFrameStruct));
	md3dImmediateContext->Unmap(perFrameBuffer, 0);
 
	//ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light2TexTech;

	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	// Draw the box.
	DirectX::XMMATRIX world = XMLoadFloat4x4(&mBoxWorld);
	DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	DirectX::XMMATRIX worldViewProj = world*view*proj;
	DirectX::XMMATRIX centerTex = DirectX::XMMatrixTranslation(-0.5f, -0.5f, 0);
	DirectX::XMMATRIX unCenterTex = DirectX::XMMatrixTranslation(0.5f, 0.5f, 0);
	DirectX::XMMATRIX texRotation = XMLoadFloat4x4(&mTexTransform) * centerTex * DirectX::XMMatrixRotationZ(rotationDegrees) * unCenterTex;
	perObjectStruct.World = DirectX::XMMatrixTranspose(world);
	perObjectStruct.WorldInvTranspose = DirectX::XMMatrixTranspose(worldInvTranspose);
	perObjectStruct.WorldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	perObjectStruct.TexTransform = DirectX::XMMatrixTranspose(texRotation);
	perObjectStruct.Material = mBoxMat;

	md3dImmediateContext->Map(perObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
	memcpy(cbData.pData, &perObjectStruct, sizeof(perObjectStruct));
	md3dImmediateContext->Unmap(perObjectBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);
	md3dImmediateContext->PSSetShaderResources(0, 1, &mFlareSRV);
	md3dImmediateContext->PSSetShaderResources(1, 1, &mFlareAlphaSRV);
	md3dImmediateContext->PSSetShaderResources(2, 1, &mFireAnimSRV[frame/10]);
	md3dImmediateContext->PSSetSamplers(0, 1, &samplerState);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->VSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->PSSetConstantBuffers(0, 1, &perFrameBuffer);
	md3dImmediateContext->PSSetConstantBuffers(1, 1, &perObjectBuffer);
	md3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

	HR(mSwapChain->Present(0, 0));
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		mRadius = MathHelper::Clamp(mRadius, 1.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CrateApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	mBoxVertexOffset      = 0;

	// Cache the index count of each object.
	mBoxIndexCount      = box.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	mBoxIndexOffset      = 0;
	
	UINT totalVertexCount = box.Vertices.size();

	UINT totalIndexCount = mBoxIndexCount;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex::Basic32> vertices(totalVertexCount);

	UINT k = 0;
	for(size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos    = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].Tex    = box.Vertices[i].TexC;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::Basic32) * totalVertexCount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}
 
