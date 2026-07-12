#import "VideoRenderer.h"
#import <Metal/Metal.h>

// Shader mínimo: quad em triangle strip, amostragem nearest.
// Compilado em runtime (newLibraryWithSource) — o build Theos/Linux não tem
// o compilador Metal offline; em iOS a compilação na 1ª execução é barata.
static NSString *const kFonteShader = @""
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"struct VOut { float4 pos [[position]]; float2 uv; };\n"
"vertex VOut v_main(uint vid [[vertex_id]], constant float2 &escala [[buffer(0)]]) {\n"
"    float2 quad[4] = { float2(-1,-1), float2(1,-1), float2(-1,1), float2(1,1) };\n"
"    float2 uvs[4]  = { float2(0,1),  float2(1,1),  float2(0,0),  float2(1,0) };\n"
"    VOut o; o.pos = float4(quad[vid] * escala, 0, 1); o.uv = uvs[vid]; return o;\n"
"}\n"
"fragment float4 f_main(VOut e [[stage_in]], texture2d<float> tex [[texture(0)]]) {\n"
"    constexpr sampler s(mag_filter::nearest, min_filter::nearest);\n"
"    return tex.sample(s, e.uv);\n"
"}\n";

@implementation VideoRenderer {
    CAMetalLayer *_camada;
    id<MTLDevice> _device;
    id<MTLCommandQueue> _fila;
    id<MTLRenderPipelineState> _pipeline;
    id<MTLTexture> _textura;
    int _largura, _altura;
}

- (instancetype)initComCamada:(CAMetalLayer *)camada largura:(int)largura altura:(int)altura {
    if ((self = [super init])) {
        _camada = camada;
        _largura = largura;
        _altura = altura;
    }
    return self;
}

- (BOOL)preparar:(NSError **)erro {
    _device = MTLCreateSystemDefaultDevice();
    if (!_device) {
        if (erro) *erro = [NSError errorWithDomain:@"VideoRenderer" code:1
            userInfo:@{NSLocalizedDescriptionKey: @"Metal indisponível neste aparelho"}];
        return NO;
    }
    _camada.device = _device;
    _camada.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _camada.framebufferOnly = YES;

    _fila = [_device newCommandQueue];

    id<MTLLibrary> lib = [_device newLibraryWithSource:kFonteShader options:nil error:erro];
    if (!lib) return NO;
    MTLRenderPipelineDescriptor *desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = [lib newFunctionWithName:@"v_main"];
    desc.fragmentFunction = [lib newFunctionWithName:@"f_main"];
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    _pipeline = [_device newRenderPipelineStateWithDescriptor:desc error:erro];
    if (!_pipeline) return NO;

    MTLTextureDescriptor *td = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
        width:(NSUInteger)_largura height:(NSUInteger)_altura mipmapped:NO];
    td.usage = MTLTextureUsageShaderRead;
    _textura = [_device newTextureWithDescriptor:td];
    return _textura != nil;
}

- (void)apresentarQuadro:(RRQuadroVideo)quadro {
    if (!_pipeline || !quadro.pixels) return;
    @autoreleasepool {
        id<CAMetalDrawable> drawable = [_camada nextDrawable];
        if (!drawable) return;

        [_textura replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)_largura, (NSUInteger)_altura)
                    mipmapLevel:0
                      withBytes:quadro.pixels
                    bytesPerRow:(NSUInteger)quadro.pitchBytes];

        // escala inteira: maior fator que cabe no drawable, com barras (§7.1)
        CGSize ds = _camada.drawableSize;
        float fator = (float)MIN(floor(ds.width / _largura), floor(ds.height / _altura));
        if (fator < 1.0f) fator = 1.0f;
        float escala[2] = {
            (float)(_largura * fator / ds.width),
            (float)(_altura * fator / ds.height)
        };

        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = drawable.texture;
        rp.colorAttachments[0].loadAction = MTLLoadActionClear;
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        rp.colorAttachments[0].clearColor = MTLClearColorMake(0.04, 0.04, 0.05, 1.0);

        id<MTLCommandBuffer> cb = [_fila commandBuffer];
        id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
        [enc setRenderPipelineState:_pipeline];
        [enc setVertexBytes:escala length:sizeof escala atIndex:0];
        [enc setFragmentTexture:_textura atIndex:0];
        [enc drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [enc endEncoding];
        [cb presentDrawable:drawable];
        [cb commit];
    }
}

@end
