#include "stdafx.h"

LPDIRECT3D8 g_pD3D;
LPDIRECT3DDEVICE8 g_pd3dDevice;
LPDIRECT3DVERTEXBUFFER8 g_VertexBuffer;
D3DCAPS8 DeviceCaps;

TColorTextures g_ColorTexture;
LPDIRECT3DTEXTURE8 texBackground[3];
U32 LastDXErrorCode;

bool bFullScreen;

DWORD CompileShader(char *sPS)
{
	DWORD pPixelShader=0;
	LPD3DXBUFFER pCode;
	LPD3DXBUFFER pErrors;

	if (D3DXAssembleShader(sPS, strlen(sPS), 0, NULL, &pCode, &pErrors)!=D3D_OK){
		ShowError((char*)pErrors->GetBufferPointer());
	}else{
		SafeDX(g_pd3dDevice->CreatePixelShader((DWORD*)pCode->GetBufferPointer(),
											&pPixelShader));
	}
	if (pCode){
		SafeDX(pCode->Release());
		pCode = NULL;
	}
	if (pErrors){
		SafeDX(pErrors->Release());
		pErrors = NULL;
	}
	return pPixelShader;
}

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
		char s[512];
		strcpy(s, "Direct3D Device initialization failed. This program requires DirectX8, which is available for Windows 95/98/ME, 2000 and XP. Windows NT is not supported, sorry. You can get the newest DirectX from http://www.microsoft.com/directx. If this doesn't help you might want to try upgrading your graphics drivers.");
		ShowError(s);
		MessageBox(Global_Gfx_Info.hWnd, s, "Error", MB_OK);
		return E_FAIL;
	}
	// Save viewport data...
	SafeDX(g_pd3dDevice->GetViewport(&State.Viewport));
	State.Viewport.MinZ = 0.0f;
	State.Viewport.MaxZ = 1.0f;
	State.WindowWidth = State.Viewport.Width;
	State.WindowHeight = State.Viewport.Height;
	SafeDX(g_pd3dDevice->SetViewport(&State.Viewport));
	RECT r;
	GetClientRect(Global_Gfx_Info.hWnd, &r);

	SafeDX(g_pd3dDevice->GetDeviceCaps(&DeviceCaps));

	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE));
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE));
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 1));
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL));
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD));
	//SafeDX(g_pd3dDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE));

	U32 i;
	for (i=0;i<8;i++){
		SafeDX(g_pd3dDevice->SetTextureStageState(i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR));
		SafeDX(g_pd3dDevice->SetTextureStageState(i, D3DTSS_MINFILTER, D3DTEXF_LINEAR));
	}
//	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

	// Create the vertex buffer
	SafeDX(g_pd3dDevice->CreateVertexBuffer(
			sizeof(D3DTNTVERTEX)*D3DVertexCacheSize,
			D3DUSAGE_WRITEONLY,
			MY_VERTEX_TYPE,
			D3DPOOL_SYSTEMMEM,
			&g_VertexBuffer));
	SafeDX(g_pd3dDevice->SetStreamSource(
			0,
			g_VertexBuffer,
			sizeof(D3DTNTVERTEX)));
	SafeDX(g_pd3dDevice->SetVertexShader(MY_VERTEX_TYPE));

	// Create color textures - we use them to fake color blending if tfactor is used
	SafeDX(g_pd3dDevice->CreateTexture(YoshiBGWidth, YoshiBGHeight, 1, 0, D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &texBackground[0]));
	SafeDX(g_pd3dDevice->CreateTexture(YoshiBGWidth, YoshiBGHeight, 1, 0, D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &texBackground[1]));
	SafeDX(g_pd3dDevice->CreateTexture(YoshiBGWidth, YoshiBGHeight, 1, 0, D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &texBackground[2]));
	for (i=0;i<8;i++){
		SafeDX(g_pd3dDevice->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &g_ColorTexture[i].D3DObject));
		g_ColorTexture[i].Color = 0;
	}
//	SafeDX(g_pd3dDevice->CreateRenderTarget(512, 256, d3ddm.Format, D3DMULTISAMPLE_NONE, TRUE, &surWriteback));
	SafeDX(g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));
	
	// Create some standard combiners...
	// Highlight (debug)
	State.ps_highlight = CompileShader("ps.1.1\nmov r0,c6\n");
	State.ps_diffuseonly = CompileShader("ps.1.1\nmov r0,v0");
	State.ps_textureonly = CompileShader("ps.1.1\ntex t0\nmov r0,t0");
	State.ps_texcoloronly = CompileShader("ps.1.1\ntex t0\nmov r0,t0\nmov r0.a,c5");
	State.ps_texalphaonly = CompileShader("ps.1.1\ntex t0\nmov r0,t0.a\nmov r0.a,c5");
	
	
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
	for (i=0;i<LoggedCombineModes.Count;i++){
		if (LoggedCombineModes.Modes[i].pixelshaderhandle) { SafeDX(g_pd3dDevice->DeletePixelShader(LoggedCombineModes.Modes[i].pixelshaderhandle)); LoggedCombineModes.Modes[i].pixelshaderhandle = 0; }
	}
	if (State.ps_texalphaonly) { SafeDX(g_pd3dDevice->DeletePixelShader(State.ps_texalphaonly)); State.ps_texalphaonly = 0; }
	if (State.ps_texcoloronly) { SafeDX(g_pd3dDevice->DeletePixelShader(State.ps_texcoloronly)); State.ps_texcoloronly = 0; }
	if (State.ps_diffuseonly) { SafeDX(g_pd3dDevice->DeletePixelShader(State.ps_diffuseonly)); State.ps_diffuseonly = 0; }
	if (State.ps_highlight) { SafeDX(g_pd3dDevice->DeletePixelShader(State.ps_highlight)); State.ps_highlight = 0; }
	if (State.ps_textureonly) { SafeDX(g_pd3dDevice->DeletePixelShader(State.ps_textureonly)); State.ps_textureonly = 0; }
	LoggedCombineModes.Count = 0;
//	FreeObject(surWriteback);
	FreeObject(texBackground[2]);
	FreeObject(texBackground[1]);
	FreeObject(texBackground[0]);
	FreeObject(g_VertexBuffer);
	FreeObject(g_pd3dDevice);
	FreeObject(g_pD3D);
}
