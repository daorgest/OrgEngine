#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Core/PrimTypes.h"

class Camera
{
public:
	glm::vec3 velocity;
	glm::vec3 position;

	f32 pitch = 0.0f;
	f32 yaw = 0.f;

	glm::mat4 view = { 1.0f };
	glm::mat4 projection = { 1.0f };

	void ApplyMovement(glm::vec3 movement);

	[[nodiscard]] glm::mat4 GetViewMatrix() const;
	[[nodiscard]] glm::mat4 GetRotationMatrix() const;

	void Update(f32 deltaTime);
};
