#include "stdafx.h"

LPDIRECT3D8 g_pD3D;
LPDIRECT3DDEVICE8 g_pd3dDevice;
LPDIRECT3DVERTEXBUFFER8 g_VertexBuffer;
D3DCAPS8 DeviceCaps;

TColorTextures g_ColorTexture;
LPDIRECT3DTEXTURE8 texBackground[3];
U32 LastDXErrorCode;

bool bFullScreen;

int InitializeDirect3D()
{
	LPDIRECT3D8 g_pD3D;
	D3DPRESENT_PARAMETERS d3dpp; 
	D3DDEVTYPE DevType; // Reference, HAL, SW
	D3DDISPLAYMODE d3ddm;
	U32 AdapterID;

	
	if( NULL == ( g_pD3D = Direct3DCreate8(D3D_SDK_VERSION))){
		ShowError("Error initializing DirectX8. This program requires DirectX8, which is available for Windows 95/98/ME, 2000 and XP. Windows NT is not supported, sorry.");
		return E_FAIL;
	}

	DevType = (D3DDEVTYPE)NemuGetProfileInt("GFX", "DevType", 1, IniFilename);
	AdapterID = NemuGetProfileInt("GFX", "Adapter", 0, IniFilename);

	
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.Windowed   = !bFullScreen;
	d3dpp.EnableAutoDepthStencil = TRUE;

	// Get Backbuffer format from current settings
#line 38 "Direct3d.cpp"
	SafeDX(g_pD3D->GetAdapterDisplayMode(AdapterID, &d3ddm ) );
	d3dpp.BackBufferFormat = d3ddm.Format;
	if (bFullScreen){
		d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;;
		d3dpp.BackBufferWidth = uRenderWidth;// NemuGetProfileInt("GFX", "Width", 640, IniFilename);
		d3dpp.BackBufferHeight = uRenderHeight;//NemuGetProfileInt("GFX", "Height", 480, IniFilename);
		d3dpp.FullScreen_RefreshRateInHz = NemuGetProfileInt("GFX", "RefreshRate", 0, IniFilename);
	} else {
		d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	}

	if (FAILED(g_pD3D->CreateDevice(AdapterID, DevType, Global_Gfx_Info.hWnd,
								  D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED , 
								  &d3dpp, &g_pd3dDevice ))){
		char s[256];
		strcpy(s, "Direct3D Device initialization failed. This program requires DirectX8, which is available for Windows 95/98/ME, 2000 and XP. Windows NT is not supported, sorry. You can get the newest DirectX from http://www.microsoft.com/directx. If this doesn't help you might want to try upgrading your graphics drivers.");
		ShowError(s);
		MessageBox(Global_Gfx_Info.hWnd, s, "Error", MB_OK);
		return E_FAIL;
	}
	// Save viewport data...
#line 59 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->GetViewport(&State.Viewport));
	State.Viewport.MinZ = 0.0f;
	State.Viewport.MaxZ = 1.0f;
	State.WindowWidth = State.Viewport.Width;
	State.WindowHeight = State.Viewport.Height;
#line 64 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetViewport(&State.Viewport));
	RECT r;
	GetClientRect(Global_Gfx_Info.hWnd, &r);

#line 68 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->GetDeviceCaps(&DeviceCaps));

#line 70 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
#line 71 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE));
#line 72 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
#line 73 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE));
#line 74 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 1));
#line 75 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL));
#line 76 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD));
	//SafeDX(g_pd3dDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE));

	U32 i;
	for (i=0;i<8;i++){
#line 81 "Direct3d.cpp"
		SafeDX(g_pd3dDevice->SetTextureStageState(i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR));
#line 82 "Direct3d.cpp"
		SafeDX(g_pd3dDevice->SetTextureStageState(i, D3DTSS_MINFILTER, D3DTEXF_LINEAR));
	}
//	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

	// Create the vertex buffer
#line 87 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->CreateVertexBuffer( sizeof(D3DTNTVERTEX)*D3DVertexCacheSize, D3DUSAGE_WRITEONLY, MY_VERTEX_TYPE, D3DPOOL_SYSTEMMEM, &g_VertexBuffer));
#line 88 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetStreamSource( 0, g_VertexBuffer, sizeof(D3DTNTVERTEX)));
#line 89 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetVertexShader(MY_VERTEX_TYPE));

	// Create color textures - we use them to fake color blending if tfactor is used
#line 92 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->CreateTexture(YoshiBGWidth, YoshiBGHeight, 1, 0, D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &texBackground[0]));
#line 93 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->CreateTexture(YoshiBGWidth, YoshiBGHeight, 1, 0, D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &texBackground[1]));
#line 94 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->CreateTexture(YoshiBGWidth, YoshiBGHeight, 1, 0, D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &texBackground[2]));
	for (i=0;i<8;i++){
#line 96 "Direct3d.cpp"
		SafeDX(g_pd3dDevice->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &g_ColorTexture[i].D3DObject));
		g_ColorTexture[i].Color = 0;
	}
//	SafeDX(g_pd3dDevice->CreateRenderTarget(512, 256, d3ddm.Format, D3DMULTISAMPLE_NONE, TRUE, &surWriteback));
#line 100 "Direct3d.cpp"
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));
	
	return NOERROR;
}

void FreeTextures()
{
	U32 i;
	if (State.TextureCount)
		for (i=0;i<State.TextureCount;i++){
			FreeObject(State.Textures[i].D3DObject);
		}
	State.TextureCount = 0;

}

void FreeDirect3D()
{
	FreeTextures();
	U32 i;
	for (i=0;i<8;i++){
		FreeObject(g_ColorTexture[i].D3DObject);
	}
//	FreeObject(surWriteback);
	FreeObject(texBackground[2]);
	FreeObject(texBackground[1]);
	FreeObject(texBackground[0]);
	FreeObject(g_VertexBuffer);
	FreeObject(g_pd3dDevice);
	FreeObject(g_pD3D);
}
