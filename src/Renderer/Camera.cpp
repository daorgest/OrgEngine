//
// Created by Orgest on 10/6/2024.
//

#include "Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

// Updates the view matrix and camera position each frame
void Camera::Update(f32 deltaTime)
{
	glm::mat4 cameraRotation = GetRotationMatrix();

	position += glm::vec3(cameraRotation * glm::vec4(velocity * deltaTime *  0.5f, 0.f));
}


// Returns the view matrix based on the camera's position and direction
glm::mat4 Camera::GetViewMatrix() const
{
	glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
	glm::mat4 cameraRotation = GetRotationMatrix();
	return glm::inverse(cameraTranslation * cameraRotation);
}

// Returns the camera's rotation matrix (if needed)
glm::mat4 Camera::GetRotationMatrix() const
{

	glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3 { 1.f, 0.f, 0.f });
	glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3 { 0.f, -1.f, 0.f });

	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}