// Big thanks to Martins for the D3D11 gist this is based on
// https://gist.github.com/mmozeiko/5e727f845db182d468a34d524508ad5f
// also this is a nice intro: https://www.3dgep.com/introduction-to-directx-11/

// TODO -- maybe we need a win32_d3d11 .c with all D3D11 init stuff and then 
// a separate file containing an abstraction that works for the gui

#include "gui.h"
guiState my_gui;

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <stddef.h>
#include <intrin.h>
#define Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define AssertHR(hr) Assert(SUCCEEDED(hr))

#pragma comment (lib, "gdi32")
#pragma comment (lib, "user32")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

//maybe all the gui stuff should be somewhere else
#pragma comment (lib, "../../gui/.build/gui.lib")

#define STR2(x) #x
#define STR(x) STR2(x)

typedef struct {
    // Universal
    HWND window;
    DWORD currentWidth;
    DWORD currentHeight;
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    IDXGISwapChain1 *swapChain;
    ID3D11RenderTargetView *rtView;
    ID3D11DepthStencilView *dsView;
    // UI specific
    ID3D11Buffer* instancebuf; // dynamic vbo holding all instance data
    ID3D11InputLayout* layout;
    ID3D11VertexShader* vshader;
    ID3D11PixelShader* pshader;
    ID3D11Buffer* ubuffer;
    ID3D11ShaderResourceView* textureView; // the font atlas texture
    ID3D11SamplerState* samplerState;
    ID3D11BlendState* blendState;
    ID3D11RasterizerState* rasterizerState;
    ID3D11DepthStencilState* depthState;
} dxBackend;

dxBackend dxb;

typedef struct
{
    float pos0[2];
    float pos1[2];
    float uv0[2];
    float uv1[2];
    float color[4];
}InstanceData;
static void FatalError(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    guiInputEvent e;
    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_MOUSEMOVE:
            s32 x_pos_abs = LOWORD(lparam);
            s32 y_pos_abs = HIWORD(lparam);
            e.type = GUI_INPUT_EVENT_TYPE_MOUSE_MOVE;
            e.param0 = *((u32*)((void*)(&x_pos_abs)));
            e.param1 = *((u32*)((void*)(&y_pos_abs)));
            gui_input_push_event(&my_gui, e);
            break;
        case WM_LBUTTONDOWN:
            e.param0=GUI_LMB;e.param1=1;e.type=GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT;
            gui_input_push_event(&my_gui, e);
            break;
        case WM_RBUTTONDOWN:
            e.param0=GUI_RMB;e.param1=1;e.type=GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT;
            gui_input_push_event(&my_gui, e);
            break;
        case WM_MBUTTONDOWN:
            e.param0=GUI_MMB;e.param1=1;e.type=GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT;
            gui_input_push_event(&my_gui, e);
            break;
        case WM_LBUTTONUP:
            e.param0=GUI_LMB;e.param1=0;e.type=GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT;
            gui_input_push_event(&my_gui, e);
            break;
        case WM_RBUTTONUP:
            e.param0=GUI_RMB;e.param1=0;e.type=GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT;
            gui_input_push_event(&my_gui, e);
            break;
        case WM_MBUTTONUP:
            e.param0=GUI_MMB;e.param1=0;e.type=GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT;
            gui_input_push_event(&my_gui, e);
            break;
    
    }
    return DefWindowProcW(wnd, msg, wparam, lparam);
}

/*
    dxb_init will allocate new window/instance/device/swap, if you want to put this in your project, 
    don't call init at all and just fill your own stuff by writing to the dxBackend struct like
    what is being done at the end of dxb_init, just put your own!
*/
void dxb_init(HINSTANCE instance, dxBackend *backend) {
    ZeroMemory(backend, sizeof(dxBackend));
    // register window class to have custom WindowProc callback
    WNDCLASSEXW wc =
    {
        .cbSize = sizeof(wc),
        .lpfnWndProc = WindowProc,
        .hInstance = instance,
        .hIcon = LoadIcon(NULL, IDI_APPLICATION),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = L"gui_sample_class",
    };
    ATOM atom = RegisterClassExW(&wc);
    Assert(atom && "Failed to register window class");

    // window properties - width, height and style
    int width = 800;
    int height = 600;
    // WS_EX_NOREDIRECTIONBITMAP flag here is needed to fix ugly bug with Windows 10
    // when window is resized and DXGI swap chain uses FLIP presentation model
    // DO NOT use it if you choose to use non-FLIP presentation model
    // read about the bug here: https://stackoverflow.com/q/63096226 and here: https://stackoverflow.com/q/53000291
    DWORD exstyle = WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP;
    DWORD style = WS_OVERLAPPEDWINDOW;

    // create window
    HWND window = CreateWindowExW(exstyle, wc.lpszClassName, L"guiSample", style,CW_USEDEFAULT, CW_USEDEFAULT, width, height,NULL, NULL, wc.hInstance, NULL);
    Assert(window && "Failed to create window");
  
    // allocate a console to log stuff
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    
    HRESULT hr;

    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // create D3D11 device & context
    {
        UINT flags = 0;
        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(
            NULL, D3D_DRIVER_TYPE_WARP, NULL, flags, levels, ARRAYSIZE(levels),
            D3D11_SDK_VERSION, &device, NULL, &context);
        AssertHR(hr);
    }

    // create DXGI swap chain
    IDXGISwapChain1* swapChain;
    {
        // get DXGI device from D3D11 device
        IDXGIDevice* dxgiDevice;
        hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void**)&dxgiDevice);
        AssertHR(hr);

        // get DXGI adapter from DXGI device
        IDXGIAdapter* dxgiAdapter;
        hr = IDXGIDevice_GetAdapter(dxgiDevice, &dxgiAdapter);
        AssertHR(hr);

        // get DXGI factory from DXGI adapter
        IDXGIFactory2* factory;
        hr = IDXGIAdapter_GetParent(dxgiAdapter, &IID_IDXGIFactory2, (void**)&factory);
        AssertHR(hr);

        DXGI_SWAP_CHAIN_DESC1 desc =
        {
            //.Width = 0,
            //.Height = 0,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { 1, 0 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .Scaling = DXGI_SCALING_NONE,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        };

        hr = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)device, window, &desc, NULL, NULL, &swapChain);
        AssertHR(hr);

        // disable silly Alt+Enter changing monitor resolution to match window size
        IDXGIFactory_MakeWindowAssociation(factory, window, DXGI_MWA_NO_ALT_ENTER);

        IDXGIFactory2_Release(factory);
        IDXGIAdapter_Release(dxgiAdapter);
        IDXGIDevice_Release(dxgiDevice);
    }
    ShowWindow(backend->window, SW_SHOWDEFAULT);
    
    backend->window = window;
    backend->device = device;
    backend->context = context;
    backend->swapChain = swapChain;
}

void dxb_prepare_ui_stuff(dxBackend *backend) {
    

    HRESULT hr;
    ID3D11Buffer* instancebuf;
    {
        
        InstanceData data[] =
        {
            {{0,0},{0,0},{0,0},{0,0},{1,1,1,1}},
            {{0,0},{0,0},{0,0},{0,0},{1,1,1,1}},
            {{0,0},{0,0},{0,0},{0,0},{1,1,1,1}},
            {{0,0},{0,0},{0,0},{0,0},{1,1,1,1}},
            {{0,0},{0,0},{0,0},{0,0},{1,1,1,1}},
            {{0,0},{0,0},{0,0},{0,0},{1,1,1,1}},
        };


        D3D11_BUFFER_DESC desc =
        {
            .ByteWidth = sizeof(data),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };

        D3D11_SUBRESOURCE_DATA initial = { .pSysMem = data };
        ID3D11Device_CreateBuffer(backend->device, &desc, &initial, &instancebuf);
    }
    
    // vertex & pixel shaders for drawing the ui, plus input layout for vertex input
    ID3D11InputLayout* layout;
    ID3D11VertexShader* vshader;
    ID3D11PixelShader* pshader;
    {
        // these must match vertex shader input layout (VS_INPUT in vertex shader source below)
        D3D11_INPUT_ELEMENT_DESC desc[] =
        {
            { "POSITION",    0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(InstanceData, pos0),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "POSITION",    1, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(InstanceData, pos1),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(InstanceData, uv0),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD",    1, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(InstanceData, uv1),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(InstanceData, color),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
//          { "SV_InstanceID",    0, DXGI_FORMAT_R32_UINT, 0, offsetof(struct Vertex, color),    D3D11_INPUT_PER_INDEX_DATA, 0 },
        };
        const char hlsl[] =
            "#line " STR(__LINE__) "                                  \n\n" // actual line number in this file for nicer error messages
            "                                                           \n"
            "static float2 vertices[] =\n"
            "{\n"
            "    {-1, -1},\n"
            "    {-1, +1},\n"
            "    {+1, -1},\n"
            "    {+1, +1},\n"
            "};\n"
            "                                                           \n"
            "                                                           \n"
            "struct VS_INPUT                                            \n"
            "{                                                          \n"
            "     float2 pos0 : POSITION0;                                 \n"
            "     float2 pos1 : POSITION1;                                 \n"
            "     float2 uv0 : TEXCOORD0;                                 \n"
            "     float2 uv1 : TEXCOORD1;                                 \n"
            "     float4 color : COLOR;                                 \n"
            "     uint vid : SV_VertexID;                                 \n"
            "};                                                         \n"
            "                                                           \n"
            "struct PS_INPUT                                            \n"
            "{                                                          \n"
            "  float4 pos   : SV_POSITION0;                              \n" // these names do not matter, except SV_... ones
            "  float2 uv    : TEXCOORD;                                 \n"
            "  float4 color : COLOR;                                    \n"
            "};                                                         \n"
            "                                                           \n"
            "cbuffer cbuffer0 : register(b0)                            \n" // b0 = constant buffer bound to slot 0
            "{                                                          \n"
            "    float2 winDim;                                   \n"
            "}                                                          \n"
            "                                                           \n"
            "sampler sampler0 : register(s0);                           \n" // s0 = sampler bound to slot 0
            "                                                           \n"
            "Texture2D<float4> texture0 : register(t0);                 \n" // t0 = shader resource bound to slot 0
            "                                                           \n"
            "PS_INPUT vs(VS_INPUT input)\n"
            "{                                                          \n"
            "    PS_INPUT output;                                       \n"
            "    float2 dst_half_size = (input.pos1 - input.pos0)/2; \n"
            "    float2 dst_center = (input.pos1 + input.pos0)/2; \n"
            "    float2 dst_pos = vertices[input.vid]*dst_half_size+dst_center; \n"
            "    output.pos = float4(2*dst_pos/winDim - 1, 0, 1); \n"
            "    output.pos.y *= -1; \n"
            "    float2 uv_half_size = (input.uv1 - input.uv0)/2; \n"
            "    float2 uv_center = (input.uv1 + input.uv0)/2; \n"
            "    float2 uv_pos = vertices[input.vid]*uv_half_size+uv_center; \n"
            "    output.uv = uv_pos; \n"
            "    //output.uv.y *= -1; \n"
            "    //float2 real_uv = float2(0,0); \n"
            "    //output.uv = real_uv;                             \n"
            "    output.color = input.color;                 \n"
            "    return output;                                         \n"
            "}                                                          \n"
            "                                                           \n"
            "float4 ps(PS_INPUT input) : SV_TARGET                      \n"
            "{                                                          \n"
            "    uint w,h,l;texture0.GetDimensions(0,w,h,l);      \n"
            "    float col = texture0.Sample(sampler0, input.uv/float2(w,h)).r;      \n"
            "    float4 tex = float4(col,col,col,col);      \n"
            "    return input.color * tex;                              \n"
            "}                                                          \n";
        ;

        UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;

        ID3DBlob* error;

        ID3DBlob* vblob;
        hr = D3DCompile(hlsl, sizeof(hlsl), NULL, NULL, NULL, "vs", "vs_5_0", flags, 0, &vblob, &error);
        if (FAILED(hr))
        {
            const char* message = ID3D10Blob_GetBufferPointer(error);
            printf(message);
            Assert(!"Failed to compile vertex shader!");
        }

        ID3DBlob* pblob;
        hr = D3DCompile(hlsl, sizeof(hlsl), NULL, NULL, NULL, "ps", "ps_5_0", flags, 0, &pblob, &error);
        if (FAILED(hr))
        {
            const char* message = ID3D10Blob_GetBufferPointer(error);
            printf(message);
            Assert(!"Failed to compile pixel shader!");
        }

        ID3D11Device_CreateVertexShader(backend->device, ID3D10Blob_GetBufferPointer(vblob), ID3D10Blob_GetBufferSize(vblob), NULL, &vshader);
        ID3D11Device_CreatePixelShader(backend->device, ID3D10Blob_GetBufferPointer(pblob), ID3D10Blob_GetBufferSize(pblob), NULL, &pshader);
        hr = ID3D11Device_CreateInputLayout(backend->device, desc, ARRAYSIZE(desc), ID3D10Blob_GetBufferPointer(vblob), ID3D10Blob_GetBufferSize(vblob), &layout);
        if (FAILED(hr))
        {
            printf("error: 0x%08x", hr);
            Assert(!"Failed to make shader layout!");
        }

        ID3D10Blob_Release(pblob);
        ID3D10Blob_Release(vblob);
    }

    ID3D11Buffer* ubuffer;
    {
        D3D11_BUFFER_DESC desc =
        {
            // space for 4x4 float matrix (cbuffer0 from pixel shader)
            .ByteWidth = 4 * 4 * sizeof(float),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        ID3D11Device_CreateBuffer(backend->device, &desc, NULL, &ubuffer);
    }

    
    ID3D11ShaderResourceView* textureView;
    {           
        gui_state_init(&my_gui);
		u8 *pixels = my_gui.atlas.tex.data;
        // the first pixel will be white
        pixels[0] = 0xFF;
		
        UINT width = 1024;
        UINT height = 1024;

        D3D11_TEXTURE2D_DESC desc =
        {
            .Width = width,
            .Height = height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8_UNORM,
            .SampleDesc = { 1, 0 },
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        };

        D3D11_SUBRESOURCE_DATA data =
        {
            .pSysMem = pixels,
            .SysMemPitch = width * sizeof(u8),
        };

        ID3D11Texture2D* texture;
        ID3D11Device_CreateTexture2D(backend->device, &desc, &data, &texture);
        ID3D11Device_CreateShaderResourceView(backend->device, (ID3D11Resource*)texture, NULL, &textureView);
        ID3D11Texture2D_Release(texture);
    }

    ID3D11SamplerState* samplerState;
    {
        D3D11_SAMPLER_DESC desc =
        {
            .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
            .MipLODBias = 0,
            .MaxAnisotropy = 1,
            .MinLOD = 0,
            .MaxLOD = D3D11_FLOAT32_MAX,
        };

        ID3D11Device_CreateSamplerState(backend->device, &desc, &samplerState);
    }


    ID3D11BlendState* blendState;
    {
        // enable alpha blending
        D3D11_BLEND_DESC desc =
        {
            .RenderTarget[0] =
            {
                .BlendEnable = TRUE,
                .SrcBlend = D3D11_BLEND_SRC_ALPHA,
                .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOp = D3D11_BLEND_OP_ADD,
                .SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
                .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
                .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
            },
        };
        ID3D11Device_CreateBlendState(backend->device, &desc, &blendState);
    }

    ID3D11RasterizerState* rasterizerState;
    {
        // disable culling
        D3D11_RASTERIZER_DESC desc =
        {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_NONE,
            .DepthClipEnable = TRUE,
        };
        ID3D11Device_CreateRasterizerState(backend->device, &desc, &rasterizerState);
    }

    ID3D11DepthStencilState* depthState;
    {
        // disable depth & stencil test
        D3D11_DEPTH_STENCIL_DESC desc =
        {
            .DepthEnable = FALSE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS,
            .StencilEnable = FALSE,
            .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
            // .FrontFace = ... 
            // .BackFace = ...
        };
        ID3D11Device_CreateDepthStencilState(backend->device, &desc, &depthState);
    }

    
    backend->instancebuf = instancebuf;
    backend->layout = layout;
    backend->vshader = vshader;
    backend->pshader = pshader;
    backend->ubuffer = ubuffer;
    backend->textureView = textureView;
    backend->samplerState = samplerState;
    backend->blendState = blendState;
    backend->rasterizerState = rasterizerState;
    backend->depthState = depthState;
}

void dxb_resize_if_needed(dxBackend *backend){
    HRESULT hr;
    // get current size for window client area
    RECT rect;
    GetClientRect(backend->window, &rect);
    DWORD width = rect.right - rect.left;
    DWORD height = rect.bottom - rect.top;

    // resize swap chain if needed
    if (backend->rtView == NULL || width != backend->currentWidth || height != backend->currentHeight)
    {
        if (backend->rtView)
        {
            // release old swap chain buffers
            ID3D11DeviceContext_ClearState(backend->context);
            ID3D11RenderTargetView_Release(backend->rtView);
            ID3D11DepthStencilView_Release(backend->dsView);
            backend->rtView = NULL;
        }

        // resize to new size for non-zero size
        if (width != 0 && height != 0)
        {
            hr = IDXGISwapChain1_ResizeBuffers(backend->swapChain, 0, width, height, DXGI_FORMAT_UNKNOWN, 0);
            if (FAILED(hr))
            {
                FatalError("Failed to resize swap chain!");
            }

            // create RenderTarget view for new backbuffer texture
            ID3D11Texture2D* backbuffer;
            IDXGISwapChain1_GetBuffer(backend->swapChain, 0, &IID_ID3D11Texture2D, (void**)&backbuffer);
            ID3D11Device_CreateRenderTargetView(backend->device, (ID3D11Resource*)backbuffer, NULL, &backend->rtView);
            ID3D11Texture2D_Release(backbuffer);

            D3D11_TEXTURE2D_DESC depthDesc =
            {
                .Width = width,
                .Height = height,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_D32_FLOAT, // or use DXGI_FORMAT_D32_FLOAT_S8X24_UINT if you need stencil
                .SampleDesc = { 1, 0 },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_DEPTH_STENCIL,
            };

            // create new depth stencil texture & DepthStencil view
            ID3D11Texture2D* depth;
            ID3D11Device_CreateTexture2D(backend->device, &depthDesc, NULL, &depth);
            ID3D11Device_CreateDepthStencilView(backend->device, (ID3D11Resource*)depth, NULL, &backend->dsView);
            ID3D11Texture2D_Release(depth);
        }

        backend->currentWidth = width;
        backend->currentHeight = height;
    }
}

void dxb_render_all(dxBackend *backend) {
    HRESULT hr;
    ShowWindow(backend->window, SW_SHOWDEFAULT);
    // can render only if window size is non-zero - we must have backbuffer & RenderTarget view created
    if (backend->rtView)
    {
        // output viewport covering all client area of window
        D3D11_VIEWPORT viewport =
        {
            .TopLeftX = 0,
            .TopLeftY = 0,
            .Width = (FLOAT)backend->currentWidth,
            .Height = (FLOAT)backend->currentHeight,
            .MinDepth = 0,
            .MaxDepth = 1,
        };

        // clear screen
        FLOAT color[] = { 0.392f, 0.584f, 0.929f, 1.f };
        ID3D11DeviceContext_ClearRenderTargetView(backend->context, backend->rtView, color);
        ID3D11DeviceContext_ClearDepthStencilView(backend->context, backend->dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

        // update uniforms
        {
            float winDim[16] = {(float)backend->currentWidth, (float)backend->currentHeight};
        
            D3D11_MAPPED_SUBRESOURCE mapped;
            ID3D11DeviceContext_Map(backend->context, (ID3D11Resource*)backend->ubuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            memcpy(mapped.pData, winDim, 16*sizeof(float));
            ID3D11DeviceContext_Unmap(backend->context, (ID3D11Resource*)backend->ubuffer, 0);
        }


        // update the dynamic vertex buffer
        {
            guiBakedChar bc = gui_font_atlas_get_char(&my_gui.atlas, 'D');
            guiBakedChar bc1 = gui_font_atlas_get_char(&my_gui.atlas, 'i');
            guiBakedChar bc2 = gui_font_atlas_get_char(&my_gui.atlas, 'e');
            printf("[%i %i] [%i %i] %f %f\n",bc1.x0,bc1.y0,bc1.x1,bc1.y1, bc1.xoff, bc1.yoff);
            float char_offset_x = 100.0f;
            float char_offset_y = 100.0f;
            float starting_y_offset= bc.yoff;

            InstanceData data[] =
            {
                {{char_offset_x + bc.xoff,char_offset_y + (starting_y_offset - bc.yoff)},{char_offset_x+ bc.xoff+(bc.x1-bc.x0),char_offset_y+(bc.y1-bc.y0)+ (starting_y_offset - bc.yoff)},{0,0},{0,0},{0,0,1,1}},
                {{char_offset_x + bc.xoff,char_offset_y + (starting_y_offset - bc.yoff)},{char_offset_x+ bc.xoff+(bc.x1-bc.x0),char_offset_y+(bc.y1-bc.y0)+ (starting_y_offset - bc.yoff)},{bc.x0,bc.y0},{bc.x1,bc.y1},{0,1,1,1}},

                {{char_offset_x + bc1.xoff + bc.xadvance,char_offset_y + (bc1.yoff-starting_y_offset)},{char_offset_x + bc1.xoff +bc.xadvance+(bc1.x1-bc1.x0),char_offset_y+ (bc1.yoff-starting_y_offset)+(bc1.y1-bc1.y0)},{0,0},{0,0},{0,0,0,1}},
                {{char_offset_x + bc1.xoff + bc.xadvance,char_offset_y + (bc1.yoff-starting_y_offset)},{char_offset_x + bc1.xoff +bc.xadvance+(bc1.x1-bc1.x0),char_offset_y+ (bc1.yoff-starting_y_offset)+(bc1.y1-bc1.y0)},{bc1.x0,bc1.y0},{bc1.x1,bc1.y1},{1,1,0,1}},
                
                {{char_offset_x + bc2.xoff + bc.xadvance+ bc1.xadvance,char_offset_y + (bc2.yoff-starting_y_offset)},{char_offset_x + bc2.xoff +bc.xadvance+ bc1.xadvance+(bc2.x1-bc2.x0),char_offset_y+ (bc2.yoff-starting_y_offset)+(bc2.y1-bc2.y0)},{0,0},{0,0},{1,0.4,0,1}},
                {{char_offset_x + bc2.xoff + bc.xadvance+ bc1.xadvance,char_offset_y + (bc2.yoff-starting_y_offset)},{char_offset_x + bc2.xoff +bc.xadvance+ bc1.xadvance+(bc2.x1-bc2.x0),char_offset_y+ (bc2.yoff-starting_y_offset)+(bc2.y1-bc2.y0)},{bc2.x0,bc2.y0},{bc2.x1,bc2.y1},{0,1,1,1}},
            };

            // for (u32 i = 0; i < gui_render_cmd_buf_count(&my_gui.rcmd_buf); ++i){
            //     memcpy(&data[i],&my_gui.rcmd_buf.commands[i], sizeof(InstanceData));
            // }


            D3D11_MAPPED_SUBRESOURCE mapped;
            ID3D11DeviceContext_Map(backend->context, (ID3D11Resource*)backend->instancebuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            memcpy(mapped.pData, data, sizeof(data));
            ID3D11DeviceContext_Unmap(backend->context, (ID3D11Resource*)backend->instancebuf, 0);
        }

        // Input Assembler
        ID3D11DeviceContext_IASetInputLayout(backend->context, backend->layout);
        ID3D11DeviceContext_IASetPrimitiveTopology(backend->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        UINT stride = sizeof(InstanceData);
        UINT offset = 0;
        ID3D11DeviceContext_IASetVertexBuffers(backend->context, 0, 1, &backend->instancebuf, &stride, &offset);

        // Vertex Shader
        ID3D11DeviceContext_VSSetConstantBuffers(backend->context, 0, 1, &backend->ubuffer);
        ID3D11DeviceContext_VSSetShader(backend->context, backend->vshader, NULL, 0);

        // Rasterizer Stage
        ID3D11DeviceContext_RSSetViewports(backend->context, 1, &viewport);
        ID3D11DeviceContext_RSSetState(backend->context, backend->rasterizerState);

        // Pixel Shader
        ID3D11DeviceContext_PSSetSamplers(backend->context, 0, 1, &backend->samplerState);
        ID3D11DeviceContext_PSSetShaderResources(backend->context, 0, 1, &backend->textureView);
        ID3D11DeviceContext_PSSetShader(backend->context, backend->pshader, NULL, 0);

        // Output Merger
        ID3D11DeviceContext_OMSetBlendState(backend->context, backend->blendState, NULL, ~0U);
        ID3D11DeviceContext_OMSetDepthStencilState(backend->context, backend->depthState, 0);
        ID3D11DeviceContext_OMSetRenderTargets(backend->context, 1, &backend->rtView, backend->dsView);

        // draw 4 vertices
        ID3D11DeviceContext_DrawInstanced(backend->context, 4, 6, 0, 0);
    }

    // change to FALSE to disable vsync
    BOOL vsync = TRUE;
    hr = IDXGISwapChain1_Present(backend->swapChain, vsync ? 1 : 0, 0);
    if (hr == DXGI_STATUS_OCCLUDED)
    {
        // window is minimized, cannot vsync - instead sleep a bit
        if (vsync)
        {
            Sleep(10);
        }
    }
    else if (FAILED(hr))
    {
        FatalError("Failed to present swap chain! Device lost?");
    }
}


int WINAPI WinMain(HINSTANCE instance, HINSTANCE previnstance, LPSTR cmdline, int cmdshow)
{
    dxb_init(instance, &dxb);   
    dxb_prepare_ui_stuff(&dxb);
    
    for (;;)
    {
        // process all incoming Windows messages
        MSG msg;
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }
        gui_state_update(&my_gui);
        dxb_resize_if_needed(&dxb);
        dxb_render_all(&dxb);
    }
}
