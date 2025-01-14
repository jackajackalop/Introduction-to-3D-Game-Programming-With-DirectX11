//***************************************************************************************
// Exercise3Demo.cpp by Frank Luna (C) 2011 All Rights Reserved.
//
// Demonstrates drawing simple geometric primitives in wireframe mode.
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
#include <d3dcompiler.h>

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};

class Exercise3App : public D3DApp
{
public:
	Exercise3App(HINSTANCE hInstance);
	~Exercise3App();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene(); 

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnSpaceDown(WPARAM btnState);

private:
	void CheckCompilationErrors(ID3D10Blob* compilationMsgs);
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

private:
	ID3D11Buffer* mVB;
	ID3D11Buffer* mIB;

	ID3D11VertexShader* mVS;
	ID3D11PixelShader* mPS;
	ID3D11Buffer* mConstantBuffer;
	ID3D10Blob* mVSBlob;

	ID3D11InputLayout* mInputLayout;

	ID3D11RasterizerState* mWireframeRS;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mCenter;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	int mVertexOffset;

	UINT mIndexOffset;

	UINT mIndexCount;

	float mTheta;
	float mPhi;
	float mRadius;

	UINT primitiveType = 0;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	Exercise3App theApp(hInstance);
	
	if( !theApp.Init() )
		return 0;
	
	return theApp.Run();
}
 

Exercise3App::Exercise3App(HINSTANCE hInstance)
: D3DApp(hInstance), mVB(0), mIB(0), mInputLayout(0), mWireframeRS(0),
  mTheta(1.5f*MathHelper::Pi), mPhi(0.1f*MathHelper::Pi), mRadius(15.0f), primitiveType(0)
{
	mMainWndCaption = L"Exercise3 Demo";
	
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	XMStoreFloat4x4(&mCenter, DirectX::XMMatrixTranslation(0.0f, 2.0f, 0.0f));
}

Exercise3App::~Exercise3App()
{
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mWireframeRS);
	ReleaseCOM(mVS);
	ReleaseCOM(mPS);
	ReleaseCOM(mConstantBuffer);
	ReleaseCOM(mVSBlob);
}

bool Exercise3App::Init()
{
	if(!D3DApp::Init())
		return false;

	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = true;
	wireframeDesc.DepthClipEnable = true;

	HR(md3dDevice->CreateRasterizerState(&wireframeDesc, &mWireframeRS));

	return true;
}

void Exercise3App::OnResize()
{
	D3DApp::OnResize();

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void Exercise3App::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius*sinf(mPhi)*cosf(mTheta);
	float z = mRadius*sinf(mPhi)*sinf(mTheta);
	float y = mRadius*cosf(mPhi);

	// Build the view matrix.
	DirectX::XMVECTOR pos    = DirectX::XMVectorSet(x, y, z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up     = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void Exercise3App::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
    md3dImmediateContext->IASetPrimitiveTopology((D3D_PRIMITIVE_TOPOLOGY)(primitiveType + 1));

	md3dImmediateContext->RSSetState(mWireframeRS);

	UINT stride = sizeof(Vertex);
    UINT offset = 0;
    md3dImmediateContext->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);

	// Set constants
	
	DirectX::XMMATRIX view  = XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj  = XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX viewProj = view*proj;

	// Draw .
	DirectX::XMMATRIX world = XMLoadFloat4x4(&mCenter);
	world = DirectX::XMMatrixTranspose(world * viewProj);

	D3D11_MAPPED_SUBRESOURCE cbData;
	md3dImmediateContext->Map(mConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);

	memcpy(cbData.pData, &world, sizeof(world));

	md3dImmediateContext->Unmap(mConstantBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &mConstantBuffer);

	md3dImmediateContext->DrawIndexed(mIndexCount, mIndexOffset, mVertexOffset);

	HR(mSwapChain->Present(0, 0));
}

void Exercise3App::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void Exercise3App::OnSpaceDown(WPARAM btnState)
{
	primitiveType = (primitiveType + 1) % 5;

	BuildGeometryBuffers();
}

void Exercise3App::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void Exercise3App::OnMouseMove(WPARAM btnState, int x, int y)
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
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 200.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void Exercise3App::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData points;

	GeometryGenerator geoGen;
	geoGen.CreatePoints(10.0f, 10.0f, primitiveType, points);

	mVertexOffset = 0;
	mIndexCount = points.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	mIndexOffset = 0;

	UINT totalVertexCount = points.Vertices.size();

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex> vertices(totalVertexCount);

	DirectX::XMFLOAT4 black(0.0f, 0.0f, 0.0f, 1.0f);

	UINT k = 0;
	for (size_t i = 0; i < totalVertexCount; ++i, ++k)
	{
		vertices[k].Pos = points.Vertices[i].Position;
		vertices[k].Color = black;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//
	if(primitiveType > 0)
	{
		std::vector<UINT> indices;
		indices.insert(indices.end(), points.Indices.begin(), points.Indices.end());

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));
	}
}

void Exercise3App::CheckCompilationErrors(ID3D10Blob* compilationMsgs)
{
	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}
}

void Exercise3App::BuildFX()
{
	ID3D10Blob* compiledShader = 0;
	ID3D10Blob* compilationMsgs = 0;

    HR(D3DCompileFromFile(L"FX/color.fx", 0, 0, "VS", "vs_5_0", 0,
        0, &mVSBlob, &compilationMsgs));

    CheckCompilationErrors(compilationMsgs);

    HR(md3dDevice->CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVS));

    HR(D3DCompileFromFile(L"FX/color.fx", 0, 0, "PS", "ps_5_0", 0,
        0, &compiledShader, &compilationMsgs));

    CheckCompilationErrors(compilationMsgs);

    HR(md3dDevice->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, &mPS));
    ReleaseCOM(compiledShader);

    // Create a constant buffer for the shader constants
    D3D11_BUFFER_DESC constantBufferDesc = { sizeof(DirectX::XMMATRIX), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE };
    HR(md3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &mConstantBuffer));
}

void Exercise3App::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
	HR(md3dDevice->CreateInputLayout(vertexDesc, 2, mVSBlob->GetBufferPointer(),
		mVSBlob->GetBufferSize(), &mInputLayout));
}