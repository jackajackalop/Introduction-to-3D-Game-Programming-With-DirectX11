
#include <windows.h> // for FLOAT definition
#include <DirectXMath.h>
#include <iostream>
using namespace std;

// Overload the  "<<" operators so that we can use cout to 
// output XMVECTOR objects.
ostream& operator<<(ostream& os, DirectX::FXMVECTOR v)
{
	DirectX::XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z <<  ", " << dest.w << ")";
	return os;
}
 
int main()
{
	cout.setf(ios_base::boolalpha);

	// Check support for SSE2 (Pentium4, AMD K8, and above).
	if( !DirectX::XMVerifyCPUSupport() )
	{
		cout << "xna math not supported" << endl;
		return 0;
	}
	
	DirectX::XMVECTOR p = DirectX::XMVectorSet(2.0f, 2.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR q = DirectX::XMVectorSet(2.0f, -0.5f, 0.5f, 0.1f);
	DirectX::XMVECTOR u = DirectX::XMVectorSet(1.0f, 2.0f, 4.0f, 8.0f);
	DirectX::XMVECTOR v = DirectX::XMVectorSet(-2.0f, 1.0f, -3.0f, 2.5f);
	DirectX::XMVECTOR w = DirectX::XMVectorSet(0.0f, DirectX::XM_PIDIV4, DirectX::XM_PIDIV2, DirectX::XM_PI);

	cout << "XMVectorAbs(v)                 = " << DirectX::XMVectorAbs(v) << endl;
	cout << "XMVectorCos(w)                 = " << DirectX::XMVectorCos(w) << endl;
	cout << "XMVectorLog(u)                 = " << DirectX::XMVectorLog(u) << endl;
	cout << "XMVectorExp(p)                 = " << DirectX::XMVectorExp(p) << endl;
	
	cout << "XMVectorPow(u, p)              = " << DirectX::XMVectorPow(u, p) << endl;
	cout << "XMVectorSqrt(u)                = " << DirectX::XMVectorSqrt(u) << endl;
	
	cout << "XMVectorSwizzle(u, 2, 2, 1, 3) = " 
		<< DirectX::XMVectorSwizzle(u, 2, 2, 1, 3) << endl;
	cout << "XMVectorSwizzle(u, 2, 1, 0, 3) = " 
		<< DirectX::XMVectorSwizzle(u, 2, 1, 0, 3) << endl;

	cout << "XMVectorMultiply(u, v)         = " << DirectX::XMVectorMultiply(u, v) << endl;
	cout << "XMVectorSaturate(q)            = " << DirectX::XMVectorSaturate(q) << endl;
	cout << "XMVectorMin(p, v               = " << DirectX::XMVectorMin(p, v) << endl;
	cout << "XMVectorMax(p, v)              = " << DirectX::XMVectorMax(p, v) << endl;

	return 0;
}
