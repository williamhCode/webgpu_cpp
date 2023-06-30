#include "camera.hpp"
#include <algorithm>

namespace util {

using namespace wgpu;

Camera::Camera(
  Handle *handle,
  glm::vec3 position,
  glm::vec3 orientation,
  float fov,
  float aspect,
  float near,
  float far
)
    : m_handle(handle), position(position), orientation(orientation) {
  BufferDescriptor bufferDesc{
    .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
    .size = sizeof(glm::mat4),
  };
  uniformBuffer = handle->device.CreateBuffer(&bufferDesc);

  projection = glm::perspective(fov, aspect, near, far);
  Update();
}

void Camera::Update() {
  glm::mat4 pitch = glm::rotate(orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::mat4 roll = glm::rotate(orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 yaw = glm::rotate(orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 rotation = yaw * pitch * roll;

  glm::vec3 forward = rotation * m_forward;
  glm::vec3 up = rotation * m_up;

  view = glm::lookAt(position, position + forward, up);
  viewProj = projection * view;

  m_handle->queue.WriteBuffer(uniformBuffer, 0, &viewProj, sizeof(viewProj));
}

} // namespace util
