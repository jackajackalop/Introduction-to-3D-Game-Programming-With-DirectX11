//***************************************************************************************
// BoxDemo.cpp by Frank Luna (C) 2011 All Rights Reserved.
//
// Demonstrates rendering a colored box.
//
// Controls:
//		Hold the left mouse button down and move the mouse to rotate.
//      Hold the right mouse button down to zoom in and out.
//
//***************************************************************************************

//#define USE_FX

#include "d3dApp.h"
#include "MathHelper.h"
#include <d3dcompiler.h>
#include <DirectXPackedVector.h>

#if defined(USE_FX)
#include "d3dx11Effect.h"
#endif

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::PackedVector::XMCOLOR Color;
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	~BoxApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene(); 

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildGeometryBuffers();
	void BuildFX();
    void CheckCompilationErrors(ID3D10Blob* compilationMsgs);
	void BuildVertexLayout();

private:
	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxCB;
	ID3D11Buffer* mBoxIB;
	ID3D11RasterizerState* mWireframeRS;

#if defined(USE_FX)
	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;
#else
    ID3D11VertexShader* mVS;
    ID3D11PixelShader* mPS;
    ID3D11Buffer* mConstantBuffer;
    ID3D10Blob* mVSBlob;
#endif

	ID3D11InputLayout* mInputLayout;

	DirectX::XMFLOAT4X4 mWorld;
	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

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

	BoxApp theApp(hInstance);
	
	if( !theApp.Init() )
		return 0;
	
	return theApp.Run();
}
 

BoxApp::BoxApp(HINSTANCE hInstance)
: D3DApp(hInstance), mBoxVB(0), mBoxCB(0), mBoxIB(0), mWireframeRS(0),
#if defined(USE_FX)
    mFX(0), mTech(0), mfxWorldViewProj(0), 
#endif
    mInputLayout(0), mTheta(1.5f*MathHelper::Pi), mPhi(0.25f*MathHelper::Pi), mRadius(5.0f)
{
	mMainWndCaption = L"Box Demo";
	
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);
}

BoxApp::~BoxApp()
{
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxCB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mWireframeRS);

#if defined(USE_FX)
    ReleaseCOM(mFX);
#else
    ReleaseCOM(mVS);
    ReleaseCOM(mPS);
    ReleaseCOM(mConstantBuffer);
    ReleaseCOM(mVSBlob);
#endif
}

bool BoxApp::Init()
{
	if(!D3DApp::Init())
		return false;

	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_NONE;
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;

	HR(md3dDevice->CreateRasterizerState(&wireframeDesc, &mWireframeRS));

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::UpdateScene(float dt)
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

void BoxApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
    md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT vStride = sizeof(DirectX::XMFLOAT3);
	UINT cStride = sizeof(DirectX::PackedVector::XMCOLOR);
    UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &vStride, &offset);
	md3dImmediateContext->IASetVertexBuffers(1, 1, &mBoxCB, &cStride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	md3dImmediateContext->RSSetState(mWireframeRS);

	// Set constants
	DirectX::XMMATRIX world = XMLoadFloat4x4(&mWorld);
	DirectX::XMMATRIX view  = XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj  = XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX worldViewProj = world*view*proj;

#if defined(USE_FX)
	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

    D3DX11_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
        
		// 36 indices for the box.
		md3dImmediateContext->DrawIndexed(36, 0, 0);
    }
#else
    worldViewProj = XMMatrixTranspose(worldViewProj);

    D3D11_MAPPED_SUBRESOURCE cbData;
    md3dImmediateContext->Map(mConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);

    memcpy(cbData.pData, &worldViewProj, sizeof(worldViewProj));

    md3dImmediateContext->Unmap(mConstantBuffer, 0);

    md3dImmediateContext->VSSetShader(mVS, nullptr, 0);
    md3dImmediateContext->PSSetShader(mPS, nullptr, 0);

    md3dImmediateContext->VSSetConstantBuffers(0, 1, &mConstantBuffer);

    // 36 indices for the box.
    md3dImmediateContext->DrawIndexed(36, 0, 0);
#endif

	HR(mSwapChain->Present(0, 0));
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

static UINT ArgbToAbgr(DirectX::XMFLOAT4 argb)
{
	int A = (int)(argb.w * 255);
	int R = (int)(argb.x * 255);
	int G = (int)(argb.y * 255);
	int B = (int)(argb.z * 255);
	return (A << 24) | (B << 16) | (G << 8) | (R << 0);
}

void BoxApp::BuildGeometryBuffers()
{
	// Create vertex buffer
	DirectX::XMFLOAT3 vertices[] =
    {
		DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f),
		DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f),
		DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), 
		DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), 
		DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), 
		DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), 
		DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), 
		DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), 
    };

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(DirectX::XMFLOAT3) * 8;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = vertices;
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

	// Create color buffer
	DirectX::PackedVector::XMCOLOR colors[] =
	{
		ArgbToAbgr(DirectX::XMFLOAT4((const float*)&Colors::White)),
		ArgbToAbgr(DirectX::XMFLOAT4((const float*)&Colors::Black)),
		ArgbToAbgr(DirectX::XMFLOAT4((const float*)&Colors::Red)),
		ArgbToAbgr(DirectX::XMFLOAT4((const float*)&Colors::Green)),
		ArgbToAbgr(DirectX::XMFLOAT4((const float*)&Colors::Blue)),
		ArgbToAbgr(DirectX::XMFLOAT4((const float*)&Colors::Yellow)),
		ArgbToAbgr(DirectX::XMFLOAT4((const float*)&Colors::Cyan)),
		ArgbToAbgr(DirectX::XMFLOAT4((const float*)&Colors::Magenta))
	};

	D3D11_BUFFER_DESC cbd;
	cbd.Usage = D3D11_USAGE_IMMUTABLE;
	cbd.ByteWidth = sizeof(DirectX::PackedVector::XMCOLOR) * 8;
	cbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	cbd.CPUAccessFlags = 0;
	cbd.MiscFlags = 0;
	cbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA cinitData;
	cinitData.pSysMem = colors;
	HR(md3dDevice->CreateBuffer(&cbd, &cinitData, &mBoxCB));

	// Create the index buffer

	UINT indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3, 
		4, 3, 7
	};

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * 36;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = indices;
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}

void BoxApp::CheckCompilationErrors(ID3D10Blob* compilationMsgs)
{
    // compilationMsgs can store errors or warnings.
    if (compilationMsgs != 0)
    {
        MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
        ReleaseCOM(compilationMsgs);
    }
}
 
void BoxApp::BuildFX()
{
	DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
 
	ID3D10Blob* compiledShader = 0;
	ID3D10Blob* compilationMsgs = 0;

#if defined(USE_FX)
	HRESULT hr = D3DCompileFromFile(L"FX/color.fx", 0, 0, 0, "fx_5_0", shaderFlags, 
		0, &compiledShader, &compilationMsgs);

    CheckCompilationErrors(compilationMsgs);

	HR(D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 
		0, md3dDevice, &mFX));

	// Done with compiled shader.
	ReleaseCOM(compiledShader);

	mTech    = mFX->GetTechniqueByName("ColorTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
#else
    HR(D3DCompileFromFile(L"FX/color.fx", 0, 0, "VS", "vs_5_0", shaderFlags,
        0, &mVSBlob, &compilationMsgs));

    CheckCompilationErrors(compilationMsgs);

    HR(md3dDevice->CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVS));

    HR(D3DCompileFromFile(L"FX/color.fx", 0, 0, "PS", "ps_5_0", shaderFlags,
        0, &compiledShader, &compilationMsgs));

    CheckCompilationErrors(compilationMsgs);

    HR(md3dDevice->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, &mPS));
    ReleaseCOM(compiledShader);

    // Create a constant buffer for the shader constants
    D3D11_BUFFER_DESC constantBufferDesc = { sizeof(DirectX::XMMATRIX), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE };
    HR(md3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &mConstantBuffer));
#endif
}

void BoxApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
#if defined(USE_FX)
    D3DX11_PASS_DESC passDesc;
    mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(md3dDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature, 
		passDesc.IAInputSignatureSize, &mInputLayout));
#else
    HR(md3dDevice->CreateInputLayout(vertexDesc, 2, mVSBlob->GetBufferPointer(),
        mVSBlob->GetBufferSize(), &mInputLayout));
#endif
}
 