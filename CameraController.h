#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMode {
    FirstPerson,
    ThirdPerson
};

/*
 * ON-RAILS CAMERA CONTROLLER FOR LEVEL 1
 * 
 * Both camera modes always look straight forward along -Z direction.
 * The camera position follows the ship with smooth lerping for polished feel.
 * No mouse-look - cameras are locked to forward view.
 */

class CameraController {
public:
    CameraMode mode;
    
    // Camera position and orientation
    glm::vec3 position;
    glm::vec3 front;  // Always points along -Z (forward)
    glm::vec3 up;
    glm::vec3 right;
    
    // Level 1: Third-person camera settings (chase cam)
    float thirdPersonDistanceBack;   // How far behind the ship
    float thirdPersonHeight;         // How far above the ship
    float thirdPersonFollowLerp;     // Smooth follow speed (0-1, higher = snappier)
    
    // Level 1: First-person camera settings (cockpit view)
    float firstPersonForwardOffset;  // Forward from ship center (at nose)
    float firstPersonUpOffset;       // Up from ship center (cockpit height)
    float firstPersonFollowLerp;     // Smooth follow speed (0-1, can be snappier)
    
    CameraController();
    
    // Update camera based on ship position with smooth follow (requires dt for lerp)
    void update(const glm::vec3& shipPosition, float deltaTime);
    
    // Toggle between first-person and third-person
    void toggleMode();
    
    // Get view matrix for rendering
    glm::mat4 getViewMatrix() const;
};

#endif
