#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "ccVector.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool gWindowWantsToQuit = false;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_ESCAPE:
        {
            gWindowWantsToQuit = true;
        } break;
        }
    } break;
    case WM_SYSCOMMAND:
    {
        switch (wParam)
        {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
        {
            return 0;
        }
        }
    } break;
    case WM_CLOSE:
    {
        gWindowWantsToQuit = true;
    } break;
    }

    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

int main()
{
    const char* windowClassName = "DemosForDummiesWindowClass";

    WNDCLASSA windowClass;
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = &WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(nullptr);
    windowClass.hIcon = nullptr;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = windowClassName;
    if (!RegisterClassA(&windowClass))
    {
        return 1;
    }

    const int width = 1280;
    const int height = 720;
    const DWORD exStyle = WS_EX_APPWINDOW;
    const DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

    HWND hWnd = CreateWindowExA(exStyle, windowClassName, "Demos For Dummies", style,
        0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!hWnd)
    {
        return 1;
    }

    //////////////////////////////////////////////////////////////////////////
    
    DXGI_SWAP_CHAIN_DESC desc = { 0 };
    desc.BufferCount = 2;
    desc.BufferDesc.Width = width;
    desc.BufferDesc.Height = height;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.OutputWindow = hWnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = true;
    
    ID3D11Device * device = nullptr;
    ID3D11DeviceContext * context = nullptr;
    IDXGISwapChain * swapChain = nullptr;
    if ( D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &desc, &swapChain, &device, nullptr, &context ) != S_OK )
    {
        return 1;
    }

    ID3D11Texture2D * backBuffer = nullptr;
    ID3D11RenderTargetView * backBufferView = nullptr;
    swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void **)&backBuffer );
    device->CreateRenderTargetView( backBuffer, nullptr, &backBufferView );
   
    D3D11_TEXTURE2D_DESC depthDesc = CD3D11_TEXTURE2D_DESC( DXGI_FORMAT_R24G8_TYPELESS, width, height, 1, 1, D3D11_BIND_DEPTH_STENCIL );
    ID3D11Texture2D * depthStencil = nullptr;
    if ( device->CreateTexture2D( &depthDesc, nullptr, &depthStencil ) != S_OK )
    {
      return 1;
    }
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = CD3D11_DEPTH_STENCIL_VIEW_DESC( D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D24_UNORM_S8_UINT );
    ID3D11DepthStencilView * depthStencilView = nullptr;
    if ( device->CreateDepthStencilView( depthStencil, &descDSV, &depthStencilView ) != S_OK )
    {
      return false;
    }
 
    CD3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC( CD3D11_DEFAULT() );
    rasterizerDesc.FrontCounterClockwise = true;
    ID3D11RasterizerState * rasterizerState = nullptr;
    device->CreateRasterizerState( &rasterizerDesc, &rasterizerState );
    
    //////////////////////////////////////////////////////////////////////////
 
    cgltf_options gltfOptions = { 0 };
    cgltf_data * meshData = nullptr;
    //const char* meshFilename = "mesh.glb";
    const char* meshFilename = "sphere.glb";

    if ( cgltf_parse_file( &gltfOptions, meshFilename, &meshData ) != cgltf_result_success )
    {
        return 1;
    }
    if ( cgltf_load_buffers( &gltfOptions, meshData, meshFilename ) != cgltf_result_success )
    {
        return 1;
    }

    cgltf_primitive& primitive = meshData->meshes[0].primitives[0];

    D3D11_BUFFER_DESC bufferDesc = CD3D11_BUFFER_DESC((UINT)primitive.attributes[0].data->buffer_view->size, D3D11_BIND_VERTEX_BUFFER);
    D3D11_SUBRESOURCE_DATA subData = { 0 };
    subData.pSysMem = (char*)primitive.attributes[0].data->buffer_view->buffer->data + primitive.attributes[0].data->buffer_view->offset;
    ID3D11Buffer* vertexBuffer = nullptr;
    if (device->CreateBuffer(&bufferDesc, &subData, &vertexBuffer) != S_OK)
    {
        return 1;
    }

    D3D11_BUFFER_DESC ibBufferDesc = CD3D11_BUFFER_DESC((UINT)primitive.indices->buffer_view->size, D3D11_BIND_INDEX_BUFFER);
    D3D11_SUBRESOURCE_DATA ibSubData = { 0 };
    ibSubData.pSysMem = (char*)primitive.indices->buffer_view->buffer->data + primitive.indices->buffer_view->offset;
    ID3D11Buffer* indexBuffer = nullptr;
    if (device->CreateBuffer(&ibBufferDesc, &ibSubData, &indexBuffer) != S_OK)
    {
        return 1;
    }

    //////////////////////////////////////////////////////////////////////////

    int x, y, n;
    unsigned char* textureData = stbi_load("grass.jpg", &x, &y, &n, 4);
    if (!textureData)
    {
        return 1;
    }

    D3D11_TEXTURE2D_DESC tex2DDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_UNORM, x, y, 1, 1, D3D11_BIND_SHADER_RESOURCE);
    D3D11_SUBRESOURCE_DATA texSubData = { 0 };
    texSubData.pSysMem = textureData;
    texSubData.SysMemPitch = x * 4 * sizeof(char);
    ID3D11Texture2D* texture = nullptr;
    if (device->CreateTexture2D(&tex2DDesc, &texSubData, &texture) != S_OK)
    {
        return 1;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2D, tex2DDesc.Format, 0, 1);
    ID3D11ShaderResourceView* textureSRV = nullptr;
    if (device->CreateShaderResourceView(texture, &srvDesc, &textureSRV) != S_OK)
    {
        return 1;
    }

    //////////////////////////////////////////////////////////////////////////

    float constantBufferData[32] = { 0 };
    D3D11_BUFFER_DESC constantBufferDesc = CD3D11_BUFFER_DESC(sizeof(constantBufferData), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    D3D11_SUBRESOURCE_DATA constantSubData = { 0 };
    constantSubData.pSysMem = constantBufferData;
    ID3D11Buffer* constantBuffer = nullptr;
    if (device->CreateBuffer(&constantBufferDesc, &constantSubData, &constantBuffer) != S_OK)
    {
        return false;
    }

    char shaderSource[] = R"(
    cbuffer c
    {
      float4x4 projectionMatrix;
      float4x4 worldMatrix;
    };

    struct VS_INPUT
    {
      float3 Pos : POSITION;
    };
 
    struct VS_OUTPUT
    {
      float4 Pos : SV_POSITION;
      float2 TexCoord : TEXCOORD0;
    };
 
    VS_OUTPUT vs_main( VS_INPUT In )
    {
      VS_OUTPUT Out;
      Out.Pos = float4( In.Pos, 1.0 );
      Out.Pos = mul(worldMatrix, Out.Pos);
      Out.Pos = mul(projectionMatrix, Out.Pos);
      Out.TexCoord = In.Pos.xy + In.Pos.z;
      return Out;
    }
  
    Texture2D tex;
    SamplerState smp;
    float4 ps_main( VS_OUTPUT In ) : SV_TARGET
    {
      float4 c = 1;
      c = tex.Sample( smp, In.TexCoord );
      return c;
    }
    )";

    ID3DBlob* vertexShaderBlob = nullptr;
    if (D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vertexShaderBlob, nullptr) != S_OK)
    {
        return 1;
    }
    ID3D11VertexShader* vertexShader = nullptr;
    if (device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader) != S_OK)
    {
        return 1;
    }
    ID3DBlob* pixelShaderBlob = nullptr;
    if (D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &pixelShaderBlob, nullptr) != S_OK)
    {
        return 1;
    }
    ID3D11PixelShader* pixelShader = nullptr;
    if (device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader) != S_OK)
    {
        return 1;
    }

    static D3D11_INPUT_ELEMENT_DESC inputDesc[] =
    {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* inputLayout = nullptr;
    if (device->CreateInputLayout(inputDesc, 1, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &inputLayout) != S_OK)
    {
        return 1;
    }

    //////////////////////////////////////////////////////////////////////////
    
    ma_engine engine;
    if ( ma_engine_init( nullptr, &engine ) != MA_SUCCESS )
    {
        return 1;
    }
    
    ma_sound sound;
    if ( ma_sound_init_from_file( &engine, "audio.mp3", 0, nullptr, nullptr, &sound ) != MA_SUCCESS )
    {
        return 1;
    }
    
    ma_sound_start( &sound );

    //////////////////////////////////////////////////////////////////////////

    while (!gWindowWantsToQuit)
    {
        MSG msg;
        if (PeekMessage(&msg, hWnd, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        float cursor = 0.0f;
        ma_sound_get_cursor_in_seconds( &sound, &cursor );

        const float clearColor[ 4 ] = { 0.1f, 0.2f, 0.3f, 0.0f };
        context->ClearRenderTargetView( backBufferView, clearColor );
        context->ClearDepthStencilView( depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

        //////////////////////////////////////////////////////////////////////////

        // INSERT DEMO HERE
        mat4x4& projectionMatrix = *(mat4x4*)&constantBufferData[0];
        mat4x4Perspective(projectionMatrix, 3.1415f * 0.25f, width / (float)height, 0.01f, 10.0f);

        mat4x4& worldMatrix = *(mat4x4*)&constantBufferData[16];
        mat4x4Identity(worldMatrix);
        mat4x4RotateY(worldMatrix, cursor);
        const vec3 translation = { 0.0f, 0.0f, -5.0f };
        mat4x4Translate(worldMatrix, translation);

        //////////////////////////////////////////////////////////////////////////

        D3D11_MAPPED_SUBRESOURCE mappedSubRes;
        context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubRes);
        CopyMemory(mappedSubRes.pData, constantBufferData, constantBufferDesc.ByteWidth);
        context->Unmap(constantBuffer, 0);

        const D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.0f, 0.0f, (float)width, (float)height);
        context->RSSetViewports(1, &viewport);

        context->OMSetRenderTargets(1, &backBufferView, depthStencilView);

        context->VSSetConstantBuffers(0, 1, &constantBuffer);
        context->VSSetShader(vertexShader, nullptr, 0);
        context->PSSetShaderResources(0, 1, &textureSRV);
        context->PSSetShader(pixelShader, nullptr, 0);

        context->RSSetState(rasterizerState);
        context->IASetInputLayout(inputLayout);

        ID3D11Buffer* buffers[] = { vertexBuffer };
        const UINT stride[] = { (UINT)primitive.attributes[0].data->stride };
        const UINT offset[] = { 0 };

        context->IASetVertexBuffers(0, 1, buffers, stride, offset);

        context->IASetIndexBuffer(indexBuffer, primitive.indices->component_type == cgltf_component_type_r_32u ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        context->DrawIndexed((UINT)primitive.indices->count, 0, 0);

        //////////////////////////////////////////////////////////////////////////

        swapChain->Present( 0, 0 );
    }

    ma_sound_stop( &sound );
    ma_engine_stop( &engine );

    rasterizerState->Release();

    depthStencilView->Release();
    depthStencil->Release();
    backBufferView->Release();
    backBuffer->Release();
    textureSRV->Release();
    texture->Release();
    constantBuffer->Release();
    pixelShader->Release();
    vertexShader->Release();

    vertexBuffer->Release();
    indexBuffer->Release();
    inputLayout->Release();

    cgltf_free(meshData);

    context->Release();
    device->Release();

    DestroyWindow(hWnd);
    UnregisterClassA(windowClassName, GetModuleHandle(nullptr));

    return 0;
}

