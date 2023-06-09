#include "pipeline.hpp"
#include "util/context.hpp"
#include "util/webgpu-util.hpp"
#include "game/mesh.hpp"
#include "glm-include.hpp"
#include "game/mesh.hpp"
#include <vector>

namespace util {

using namespace wgpu;
using game::Vertex;

Pipeline::Pipeline(Context &ctx) {
  // ssao pipeline -------------------------------------------------
  ShaderModule shaderGBuffer =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/g_buffer.wgsl", ctx.device);

  // view, projection layout
  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
        .buffer{
          .type = BufferBindingType::Uniform,
          .minBindingSize = sizeof(glm::mat4),
        },
      },
      BindGroupLayoutEntry{
        .binding = 1,
        .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
        .buffer{
          .type = BufferBindingType::Uniform,
          .minBindingSize = sizeof(glm::mat4),
        },
      },
      BindGroupLayoutEntry{
        .binding = 2,
        .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
        .buffer{
          .type = BufferBindingType::Uniform,
          .minBindingSize = sizeof(glm::mat4),
        },
      },
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bgl_viewProj = ctx.device.CreateBindGroupLayout(&desc);
  }
  // texture layout
  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Fragment,
        .texture{
          .sampleType = TextureSampleType::Float,
          .viewDimension = TextureViewDimension::e2D,
        },
      },
      BindGroupLayoutEntry{
        .binding = 1,
        .visibility = ShaderStage::Fragment,
        .sampler{
          .type = SamplerBindingType::Filtering,
        },
      },
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bgl_texture = ctx.device.CreateBindGroupLayout(&desc);
  }
  // offset layout
  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Vertex,
        .buffer{
          .type = BufferBindingType::Uniform,
          .minBindingSize = sizeof(glm::vec3),
        },
      },
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bgl_offset = ctx.device.CreateBindGroupLayout(&desc);
  }

  {
    std::vector<BindGroupLayout> bindGroupLayouts{
      bgl_viewProj,
      bgl_texture,
      bgl_offset,
    };
    PipelineLayoutDescriptor layoutDesc{
      .bindGroupLayoutCount = bindGroupLayouts.size(),
      .bindGroupLayouts = bindGroupLayouts.data(),
    };
    PipelineLayout pipelineLayout = ctx.device.CreatePipelineLayout(&layoutDesc);

    // Vertex State
    std::vector<VertexAttribute> vertexAttributes = {
      VertexAttribute{
        .format = VertexFormat::Float32x3,
        .offset = offsetof(Vertex, position),
        .shaderLocation = 0,
      },
      VertexAttribute{
        .format = VertexFormat::Float32x3,
        .offset = offsetof(Vertex, normal),
        .shaderLocation = 1,
      },
      VertexAttribute{
        .format = VertexFormat::Float32x2,
        .offset = offsetof(Vertex, uv),
        .shaderLocation = 2,
      },
      VertexAttribute{
        .format = VertexFormat::Float32x2,
        .offset = offsetof(Vertex, texLoc),
        .shaderLocation = 3,
      },
    };
    VertexBufferLayout vertexBufferLayout{
      .arrayStride = sizeof(Vertex),
      .stepMode = VertexStepMode::Vertex,
      .attributeCount = vertexAttributes.size(),
      .attributes = vertexAttributes.data(),
    };
    VertexState vertexState{
      .module = shaderGBuffer,
      .entryPoint = "vs_main",
      .bufferCount = 1,
      .buffers = &vertexBufferLayout,
    };

    // Primitve State
    PrimitiveState primitiveState{
      .topology = PrimitiveTopology::TriangleList,
      .stripIndexFormat = IndexFormat::Undefined,
      .frontFace = FrontFace::CCW,
      // .cullMode = CullMode::Back,
      .cullMode = CullMode::None,
    };

    // Depth Stencil State
    DepthStencilState depthStencilState{
      .format = TextureFormat::Depth24Plus,
      .depthWriteEnabled = true,
      .depthCompare = CompareFunction::Less,
    };

    // Fragment State
    std::vector<ColorTargetState> targets{
      // position
      ColorTargetState{
        .format = TextureFormat::RGBA16Float,
      },
      // normal
      ColorTargetState{
        .format = TextureFormat::RGBA16Float,
      },
      // albedo
      ColorTargetState{
        .format = TextureFormat::BGRA8Unorm,
      },
    };
    FragmentState fragmentState{
      .module = shaderGBuffer,
      .entryPoint = "fs_main",
      .targetCount = targets.size(),
      .targets = targets.data(),
    };

    RenderPipelineDescriptor pipelineDesc{
      .layout = pipelineLayout,
      .vertex = vertexState,
      .primitive = primitiveState,
      .depthStencil = &depthStencilState,
      .fragment = &fragmentState,
    };

    rpl_gBuffer = ctx.device.CreateRenderPipeline(&pipelineDesc);
  }

  // ssao pipeline -------------------------------------------------
  ShaderModule shaderVertQuad =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/vert_quad.wgsl", ctx.device);
  ShaderModule shaderFragSsao =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/frag_ssao.wgsl", ctx.device);

  std::vector<VertexAttribute> vertexAttributes = {
    VertexAttribute{
      .format = VertexFormat::Float32x2,
      .offset = 0,
      .shaderLocation = 0,
    },
    VertexAttribute{
      .format = VertexFormat::Float32x2,
      .offset = sizeof(glm::vec2),
      .shaderLocation = 1,
    },
  };
  VertexBufferLayout quadVertexBufferLayout{
    .arrayStride = sizeof(glm::vec2) * 2,
    .stepMode = VertexStepMode::Vertex,
    .attributeCount = vertexAttributes.size(),
    .attributes = vertexAttributes.data(),
  };

  // GBuffer
  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Fragment,
        .texture{
          .sampleType = TextureSampleType::UnfilterableFloat,
          .viewDimension = TextureViewDimension::e2D,
        },
      },
      BindGroupLayoutEntry{
        .binding = 1,
        .visibility = ShaderStage::Fragment,
        .texture{
          .sampleType = TextureSampleType::UnfilterableFloat,
          .viewDimension = TextureViewDimension::e2D,
        },
      },
      BindGroupLayoutEntry{
        .binding = 2,
        .visibility = ShaderStage::Fragment,
        .texture{
          .sampleType = TextureSampleType::UnfilterableFloat,
          .viewDimension = TextureViewDimension::e2D,
        },
      },
      BindGroupLayoutEntry{
        .binding = 3,
        .visibility = ShaderStage::Fragment,
        .sampler{
          .type = SamplerBindingType::NonFiltering,
        },
      },
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bgl_gBuffer = ctx.device.CreateBindGroupLayout(&desc);
  }
  // ssao specific
  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Fragment,
        .buffer{
          .type = BufferBindingType::Uniform,
          .minBindingSize = sizeof(glm::vec4) * 64,
        },
      },
      BindGroupLayoutEntry{
        .binding = 1,
        .visibility = ShaderStage::Fragment,
        .texture{
          .sampleType = TextureSampleType::UnfilterableFloat,
          .viewDimension = TextureViewDimension::e2D,
        },
      },
      BindGroupLayoutEntry{
        .binding = 2,
        .visibility = ShaderStage::Fragment,
        .sampler{
          .type = SamplerBindingType::NonFiltering,
        },
      },
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bgl_ssaoSampling = ctx.device.CreateBindGroupLayout(&desc);
  }

  {
    std::vector<BindGroupLayout> bindGroupLayouts{
      bgl_viewProj,
      bgl_gBuffer,
      bgl_ssaoSampling,
    };
    PipelineLayoutDescriptor layoutDesc{
      .bindGroupLayoutCount = bindGroupLayouts.size(),
      .bindGroupLayouts = bindGroupLayouts.data(),
    };
    PipelineLayout pipelineLayout = ctx.device.CreatePipelineLayout(&layoutDesc);

    // Vertex State
    VertexState vertexState{
      .module = shaderVertQuad,
      .entryPoint = "vs_main",
      .bufferCount = 1,
      .buffers = &quadVertexBufferLayout,
    };

    // Primitve State
    PrimitiveState primitiveState{
      .topology = PrimitiveTopology::TriangleList,
      .frontFace = FrontFace::CCW,
      .cullMode = CullMode::None,
    };

    // Fragment State
    std::vector<ColorTargetState> targets{
      // ssao texture
      ColorTargetState{
        // .format = TextureFormat::BGRA8Unorm,
        .format = TextureFormat::R8Unorm,
      },
    };
    FragmentState fragmentState{
      .module = shaderFragSsao,
      .entryPoint = "fs_main",
      .targetCount = targets.size(),
      .targets = targets.data(),
    };

    RenderPipelineDescriptor pipelineDesc{
      .layout = pipelineLayout,
      .vertex = vertexState,
      .primitive = primitiveState,
      .fragment = &fragmentState,
    };

    rpl_ssao = ctx.device.CreateRenderPipeline(&pipelineDesc);
  }

  // blur pipeline --------------------------------------------------
  ShaderModule shaderFragBlur =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/frag_blur.wgsl", ctx.device);

  {
    std::vector<BindGroupLayoutEntry> entries{
      BindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Fragment,
        .texture{
          .sampleType = TextureSampleType::UnfilterableFloat,
          .viewDimension = TextureViewDimension::e2D,
        },
      },
      BindGroupLayoutEntry{
        .binding = 1,
        .visibility = ShaderStage::Fragment,
        .sampler{
          .type = SamplerBindingType::NonFiltering,
        },
      },
    };
    BindGroupLayoutDescriptor desc{
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bgl_ssaoTexture = ctx.device.CreateBindGroupLayout(&desc);
  }

  {
    std::vector<BindGroupLayout> bindGroupLayouts{
      bgl_ssaoTexture
    };
    PipelineLayoutDescriptor layoutDesc{
      .bindGroupLayoutCount = bindGroupLayouts.size(),
      .bindGroupLayouts = bindGroupLayouts.data(),
    };
    PipelineLayout pipelineLayout = ctx.device.CreatePipelineLayout(&layoutDesc);

    // Vertex State
    VertexState vertexState{
      .module = shaderVertQuad,
      .entryPoint = "vs_main",
      .bufferCount = 1,
      .buffers = &quadVertexBufferLayout,
    };

    // Primitve State
    PrimitiveState primitiveState{
      .topology = PrimitiveTopology::TriangleList,
      .frontFace = FrontFace::CCW,
      .cullMode = CullMode::None,
    };

    // Fragment State
    std::vector<ColorTargetState> targets{
      // ssao texture
      ColorTargetState{
        .format = TextureFormat::R8Unorm,
      },
    };
    FragmentState fragmentState{
      .module = shaderFragBlur,
      .entryPoint = "fs_main",
      .targetCount = targets.size(),
      .targets = targets.data(),
    };

    RenderPipelineDescriptor pipelineDesc{
      .layout = pipelineLayout,
      .vertex = vertexState,
      .primitive = primitiveState,
      .fragment = &fragmentState,
    };

    rpl_blur = ctx.device.CreateRenderPipeline(&pipelineDesc);
  }

  // blur pipeline --------------------------------------------------
  ShaderModule shaderFragFinal =
    util::LoadShaderModule(ROOT_DIR "/res/shaders/frag_final.wgsl", ctx.device);

  {
    std::vector<BindGroupLayout> bindGroupLayouts{
      bgl_gBuffer,
      bgl_ssaoTexture
    };
    PipelineLayoutDescriptor layoutDesc{
      .bindGroupLayoutCount = bindGroupLayouts.size(),
      .bindGroupLayouts = bindGroupLayouts.data(),
    };
    PipelineLayout pipelineLayout = ctx.device.CreatePipelineLayout(&layoutDesc);

    // Vertex State
    VertexState vertexState{
      .module = shaderVertQuad,
      .entryPoint = "vs_main",
      .bufferCount = 1,
      .buffers = &quadVertexBufferLayout,
    };

    // Primitve State
    PrimitiveState primitiveState{
      .topology = PrimitiveTopology::TriangleList,
      .frontFace = FrontFace::CCW,
      .cullMode = CullMode::None,
    };

    // Fragment State
    std::vector<ColorTargetState> targets{
      // ssao texture
      ColorTargetState{
        .format = TextureFormat::BGRA8Unorm,
      },
    };
    FragmentState fragmentState{
      .module = shaderFragFinal,
      .entryPoint = "fs_main",
      .targetCount = targets.size(),
      .targets = targets.data(),
    };

    RenderPipelineDescriptor pipelineDesc{
      .layout = pipelineLayout,
      .vertex = vertexState,
      .primitive = primitiveState,
      .fragment = &fragmentState,
    };

    rpl_final = ctx.device.CreateRenderPipeline(&pipelineDesc);
  }
}

} // namespace util
