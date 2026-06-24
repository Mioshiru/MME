#ifndef RME_CAMERA_H_
#define RME_CAMERA_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace RME::Rendering {

class Camera {
public:
    Camera() : m_projection(1.0f) {}

    void Update(int width, int height, float scrollX, float scrollY, float zoom) {
        // Orthographische Projektion: Links, Rechts, Unten, Oben
        // Wir invertieren Y, damit 0 oben ist (typisch für 2D Editoren)
        m_projection = glm::ortho(0.0f, (float)width * zoom, (float)height * zoom, 0.0f, -1.0f, 1.0f);
        
        // Translation für das Scrolling einbeziehen
        m_projection = glm::translate(m_projection, glm::vec3(-scrollX, -scrollY, 0.0f));
    }

    const float* GetProjectionPtr() const { return &m_projection[0][0]; }
    const glm::mat4& GetMatrix() const { return m_projection; }

private:
    glm::mat4 m_projection;
};

} // namespace RME::Rendering

#endif
