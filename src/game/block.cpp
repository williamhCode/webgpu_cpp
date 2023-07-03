#include "block.hpp"
#include "game/direction.hpp"
#include "util/handle.hpp"
#include "util/texture.hpp"

namespace game {

using namespace wgpu;

const std::array<BlockType, 3> g_BLOCK_TYPES = {
  BlockType{
    .id = BlockId::AIR,
  },
  BlockType{
    .id = BlockId::DIRT,
    .GetTextureLoc = [](Direction dir) -> glm::vec2 {
      return {2, 0};
    },
  },
  BlockType{
    .id = BlockId::GRASS,
    .GetTextureLoc = [](Direction dir) -> glm::vec2 {
      switch (dir) {
      case Direction::TOP:
        return {0, 0};
      case Direction::BOTTOM:
        return {2, 0};
      default:
        return {1, 0};
      }
    },
  },
};

wgpu::Texture g_blocksTexture;
wgpu::TextureView g_blocksTextureView;
wgpu::Sampler g_blocksSampler;

BindGroup InitTextures(util::Handle &handle) {
  // g_blocksTexture = util::LoadTexture(handle, ROOT_DIR "/res/blocks.png");
  g_blocksTexture = util::LoadTextureMipmap(handle, ROOT_DIR "/res/blocks.png");
  TextureViewDescriptor viewDesc{
    // anything after 5th mipmap blurs the different face textures
    // 16 x 16 textures, so len([16, 8, 4, 2, 1]) = 5
    .mipLevelCount = 5,
  };
  g_blocksTextureView = g_blocksTexture.CreateView(&viewDesc);

  SamplerDescriptor samplerDesc{
    .addressModeU = AddressMode::ClampToEdge,
    .addressModeV = AddressMode::ClampToEdge,
    .addressModeW = AddressMode::ClampToEdge,
    .magFilter = FilterMode::Nearest,
    .minFilter = FilterMode::Nearest,
    .mipmapFilter = MipmapFilterMode::Linear,
  };
  g_blocksSampler = handle.device.CreateSampler(&samplerDesc);

  // create bind group
  std::vector<BindGroupEntry> entries{
    BindGroupEntry{
      .binding = 0,
      .textureView = game::g_blocksTextureView,
    },
    BindGroupEntry{
      .binding = 1,
      .sampler = game::g_blocksSampler,
    },
  };
  BindGroupDescriptor bindGroupDesc{
    .layout = handle.pipeline.bgl_texture,
    .entryCount = entries.size(),
    .entries = entries.data(),
  };
  return handle.device.CreateBindGroup(&bindGroupDesc);
}

} // namespace game
