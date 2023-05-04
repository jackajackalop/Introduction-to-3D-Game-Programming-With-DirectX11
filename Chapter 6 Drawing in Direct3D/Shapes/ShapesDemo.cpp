//***************************************************************************************
// ShapesDemo.cpp by Frank Luna (C) 2011 All Rights Reserved.
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

class ShapesApp : public D3DApp
{
public:
	ShapesApp(HINSTANCE hInstance);
	~ShapesApp();

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
	DirectX::XMFLOAT4X4 mSphereWorld[10];
	DirectX::XMFLOAT4X4 mCylWorld[10];
	DirectX::XMFLOAT4X4 mBoxWorld;
	DirectX::XMFLOAT4X4 mGridWorld;
	DirectX::XMFLOAT4X4 mCenterSphere;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	int mBoxVertexOffset;
	int mGridVertexOffset;
	int mSphereVertexOffset;
	int mCylinderVertexOffset;

	UINT mBoxIndexOffset;
	UINT mGridIndexOffset;
	UINT mSphereIndexOffset;
	UINT mCylinderIndexOffset;

	UINT mBoxIndexCount;
	UINT mGridIndexCount;
	UINT mSphereIndexCount;
	UINT mCylinderIndexCount;

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

	ShapesApp theApp(hInstance);
	
	if( !theApp.Init() )
		return 0;
	
	return theApp.Run();
}
 

ShapesApp::ShapesApp(HINSTANCE hInstance)
: D3DApp(hInstance), mVB(0), mIB(0), mInputLayout(0), mWireframeRS(0),
  mTheta(1.5f*MathHelper::Pi), mPhi(0.1f*MathHelper::Pi), mRadius(15.0f)
{
	mMainWndCaption = L"Shapes Demo";
	
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mGridWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	DirectX::XMMATRIX boxScale = DirectX::XMMatrixScaling(2.0f, 1.0f, 2.0f);
	DirectX::XMMATRIX boxOffset = DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMStoreFloat4x4(&mBoxWorld, XMMatrixMultiply(boxScale, boxOffset));

	DirectX::XMMATRIX centerSphereScale = DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f);
	DirectX::XMMATRIX centerSphereOffset = DirectX::XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	XMStoreFloat4x4(&mCenterSphere, XMMatrixMultiply(centerSphereScale, centerSphereOffset));

	for(int i = 0; i < 5; ++i)
	{
		XMStoreFloat4x4(&mCylWorld[i*2+0], DirectX::XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&mCylWorld[i*2+1], DirectX::XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f));

		XMStoreFloat4x4(&mSphereWorld[i*2+0], DirectX::XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&mSphereWorld[i*2+1], DirectX::XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f));
	}
}

ShapesApp::~ShapesApp()
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

bool ShapesApp::Init()
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
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;
	wireframeDesc.ScissorEnable = true;

	HR(md3dDevice->CreateRasterizerState(&wireframeDesc, &mWireframeRS));

	return true;
}

void ShapesApp::OnResize()
{
	D3DApp::OnResize();

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void ShapesApp::UpdateScene(float dt)
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

void ShapesApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
    md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	md3dImmediateContext->RSSetState(mWireframeRS);

	D3D11_RECT rects = { 100, 100, 400, 400 };  
	md3dImmediateContext->RSSetScissorRects(1, &rects);

	UINT stride = sizeof(Vertex);
    UINT offset = 0;
    md3dImmediateContext->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);

	// Set constants
	
	DirectX::XMMATRIX view  = XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj  = XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX viewProj = view*proj;

	// Draw the grid.
	DirectX::XMMATRIX world = XMLoadFloat4x4(&mGridWorld);
	world = DirectX::XMMatrixTranspose(world * viewProj);

	D3D11_MAPPED_SUBRESOURCE cbData;
	md3dImmediateContext->Map(mConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);

	memcpy(cbData.pData, &world, sizeof(world));

	md3dImmediateContext->Unmap(mConstantBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &mConstantBuffer);

	md3dImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

	// Draw the box.
	world = XMLoadFloat4x4(&mBoxWorld);
	world = DirectX::XMMatrixTranspose(world * viewProj);

	md3dImmediateContext->Map(mConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);

	memcpy(cbData.pData, &world, sizeof(world));

	md3dImmediateContext->Unmap(mConstantBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &mConstantBuffer);

	md3dImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

	// Draw center sphere.
	world = XMLoadFloat4x4(&mCenterSphere);
	world = DirectX::XMMatrixTranspose(world * viewProj);

	md3dImmediateContext->Map(mConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);

	memcpy(cbData.pData, &world, sizeof(world));

	md3dImmediateContext->Unmap(mConstantBuffer, 0);

	md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
	md3dImmediateContext->PSSetShader(mPS, nullptr, 0);

	md3dImmediateContext->VSSetConstantBuffers(0, 1, &mConstantBuffer);

	md3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);

	// Draw the cylinders.
	for(int i = 0; i < 10; ++i)
	{
		world = XMLoadFloat4x4(&mCylWorld[i]);
		world = DirectX::XMMatrixTranspose(world * viewProj);

		md3dImmediateContext->Map(mConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);

		memcpy(cbData.pData, &world, sizeof(world));

		md3dImmediateContext->Unmap(mConstantBuffer, 0);

		md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
		md3dImmediateContext->PSSetShader(mPS, nullptr, 0);

		md3dImmediateContext->VSSetConstantBuffers(0, 1, &mConstantBuffer);

		md3dImmediateContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
	}

	// Draw the spheres.
	for(int i = 0; i < 10; ++i)
	{
		world = XMLoadFloat4x4(&mSphereWorld[i]);
		world = DirectX::XMMatrixTranspose(world * viewProj);

		md3dImmediateContext->Map(mConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);

		memcpy(cbData.pData, &world, sizeof(world));

		md3dImmediateContext->Unmap(mConstantBuffer, 0);

		md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
		md3dImmediateContext->PSSetShader(mPS, nullptr, 0);

		md3dImmediateContext->VSSetConstantBuffers(0, 1, &mConstantBuffer);

		md3dImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
	}

	HR(mSwapChain->Present(0, 0));
}

void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void ShapesApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData cylinder;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
	geoGen.CreateGrid(20.0f, 30.0f, 60, 40, grid);
	geoGen.CreateSphere(0.5f, 20, 20, sphere);
	//geoGen.CreateGeosphere(0.5f, 2, sphere);
	geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, cylinder);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	mBoxVertexOffset      = 0;
	mGridVertexOffset     = box.Vertices.size();
	mSphereVertexOffset   = mGridVertexOffset + grid.Vertices.size();
	mCylinderVertexOffset = mSphereVertexOffset + sphere.Vertices.size();

	// Cache the index count of each object.
	mBoxIndexCount      = box.Indices.size();
	mGridIndexCount     = grid.Indices.size();
	mSphereIndexCount   = sphere.Indices.size();
	mCylinderIndexCount = cylinder.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	mBoxIndexOffset      = 0;
	mGridIndexOffset     = mBoxIndexCount;
	mSphereIndexOffset   = mGridIndexOffset + mGridIndexCount;
	mCylinderIndexOffset = mSphereIndexOffset + mSphereIndexCount;
	
	UINT totalVertexCount = 
		box.Vertices.size() + 
		grid.Vertices.size() + 
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	UINT totalIndexCount = 
		mBoxIndexCount + 
		mGridIndexCount + 
		mSphereIndexCount +
		mCylinderIndexCount;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex> vertices(totalVertexCount);

	DirectX::XMFLOAT4 black(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMFLOAT4 red(1.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMFLOAT4 blue(0.0f, 0.0f, 1.0f, 1.0f);
	DirectX::XMFLOAT4 green(0.0f, 1.0f, 0.0f, 1.0f);

	UINT k = 0;
	for(size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos   = box.Vertices[i].Position;
		vertices[k].Color = red;
	}

	for(size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos   = grid.Vertices[i].Position;
		vertices[k].Color = black;
	}

	for(size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos   = sphere.Vertices[i].Position;
		vertices[k].Color = blue;
	}

	for(size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos   = cylinder.Vertices[i].Position;
		vertices[k].Color = green;
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

	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
	indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
	indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
	indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));
}

void ShapesApp::CheckCompilationErrors(ID3D10Blob* compilationMsgs)
{
	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}
}

void ShapesApp::BuildFX()
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

void ShapesApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
	HR(md3dDevice->CreateInputLayout(vertexDesc, 2, mVSBlob->GetBufferPointer(),
		mVSBlob->GetBufferSize(), &mInputLayout));
}