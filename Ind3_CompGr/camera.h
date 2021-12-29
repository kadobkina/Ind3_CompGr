#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

class Camera
{
public:
    glm::vec3 position; // позиция камеры
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f); // направление камеры
    glm::vec3 cameratUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight; 
	
    // углы Эйлера
    double yaw = -90.0, pitch = -40.0; 

    Camera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f))
    {
        position = pos;
        updateCamera();
    }

    // матрица вида
    glm::mat4 viewMatrix()
    {
        return glm::lookAt(position, position + direction, cameratUp);
    }

private:
    void updateCamera()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction = glm::normalize(front);
        cameraRight = glm::normalize(glm::cross(direction, cameratUp));
        cameratUp = glm::normalize(glm::cross(cameraRight, direction));
    }
};