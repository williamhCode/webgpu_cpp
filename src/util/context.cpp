#include "context.hpp"
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include "util/webgpu-util.hpp"

#include <iostream>
#include <sstream>

namespace util {

using namespace wgpu;

Context::Context(GLFWwindow *window, glm::uvec2 size) {
  // instance
  instance = CreateInstance();
  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    std::exit(1);
  }

  // surface
  surface = glfw::CreateSurfaceForWindow(instance, window);

  // adapter
  RequestAdapterOptions adapterOpts{
    .powerPreference = PowerPreference::HighPerformance
  };
  adapter = util::RequestAdapter(instance, &adapterOpts);

  // device limits
  SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);
  deviceLimits = supportedLimits.limits;

  // device
  DeviceDescriptor deviceDesc{
    .label = "My Device",
    .requiredFeaturesCount = 0,
    .requiredLimits = nullptr,
    .defaultQueue{.label = "The default queue"},
  };
  device = util::RequestDevice(adapter, &deviceDesc);
  util::SetUncapturedErrorCallback(device);

  // queue
  queue = device.GetQueue();

  // swap chain format
  swapChainFormat = TextureFormat::BGRA8Unorm;

  // swap chain
  SwapChainDescriptor swapChainDesc{
    .usage = TextureUsage::RenderAttachment,
    .format = swapChainFormat,
    .width = size.x,
    .height = size.y,
    .presentMode = PresentMode::Immediate,
  };
  swapChain = device.CreateSwapChain(surface, &swapChainDesc);

  pipeline = Pipeline(*this);
}

} // namespace util
