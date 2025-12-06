#include "CameraController.h"
#include <glm/gtc/matrix_transform.hpp>

/*
 * ═══════════════════════════════════════════════════════════════
 *  LEVEL 1: CAMERA CONFIGURATION
 * ═══════════════════════════════════════════════════════════════
 * 
 * Smooth camera follow creates polished arcade feel.
 * Camera lags slightly behind ship movement instead of snapping.
 * 
 * Tuning Guide:
 *   - Increase followLerp for snappier camera (closer to 1.0)
 *   - Decrease followLerp for floatier camera (closer to 0.0)
 *   - Adjust height/distance for better viewing angle
 */

// Level 1: Third-person camera positioning
const float L1_CAM_THIRD_DISTANCE = 12.0f;     // Behind ship (positive Z offset)
const float L1_CAM_THIRD_HEIGHT = 4.0f;        // Above ship (positive Y offset)
const float L1_CAM_THIRD_FOLLOW_LERP = 8.0f;   // Smooth follow speed

// Level 1: First-person camera positioning  
const float L1_CAM_FIRST_FORWARD = -3.0f;      // At ship nose (negative Z = forward)
const float L1_CAM_FIRST_UP = 0.5f;            // Cockpit height
const float L1_CAM_FIRST_FOLLOW_LERP = 12.0f;  // Snappier for first-person

CameraController::CameraController()
    : mode(CameraMode::ThirdPerson),
      position(0.0f, 0.0f, 0.0f),
      front(0.0f, 0.0f, -1.0f),        // Always look forward along -Z
      up(0.0f, 1.0f, 0.0f),            // World up
      right(1.0f, 0.0f, 0.0f),         // World right
      thirdPersonDistanceBack(L1_CAM_THIRD_DISTANCE),
      thirdPersonHeight(L1_CAM_THIRD_HEIGHT),
      thirdPersonFollowLerp(L1_CAM_THIRD_FOLLOW_LERP),
      firstPersonForwardOffset(L1_CAM_FIRST_FORWARD),
      firstPersonUpOffset(L1_CAM_FIRST_UP),
      firstPersonFollowLerp(L1_CAM_FIRST_FOLLOW_LERP) {
}

void CameraController::update(const glm::vec3& shipPosition, float deltaTime) {
    /*
     * ═══════════════════════════════════════════════════════════════
     *  LEVEL 1: SMOOTH ON-RAILS CAMERA UPDATE
     * ═══════════════════════════════════════════════════════════════
     * 
     * Camera smoothly follows ship with slight lag for polished feel.
     * Both modes:
     *   - Always look straight forward along -Z (no rotation)
     *   - Lerp to target position instead of snapping
     *   - Still on-rails (no free-look)
     */
    
    // Camera orientation is ALWAYS fixed forward (on-rails)
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    right = glm::vec3(1.0f, 0.0f, 0.0f);
    
    glm::vec3 targetCamPos;
    float currentFollowLerp;
    
    if (mode == CameraMode::FirstPerson) {
        /*
         * FIRST-PERSON MODE (Cockpit View)
         * - Camera at ship's nose/cockpit, looking straight ahead
         * - Slightly snappier follow for direct control feel
         * - Still on-rails (not free-look FPS)
         */
        targetCamPos = shipPosition;
        targetCamPos.z += firstPersonForwardOffset;  // At ship nose
        targetCamPos.y += firstPersonUpOffset;       // Cockpit height
        currentFollowLerp = firstPersonFollowLerp;
        
    } else {  // ThirdPerson
        /*
         * THIRD-PERSON MODE (Chase Camera)
         * - Camera behind and above the ship
         * - Smooth follow with slight lag for cinematic feel
         * - Still looking straight forward along -Z
         */
        targetCamPos = shipPosition;
        targetCamPos.z += thirdPersonDistanceBack;   // Behind ship
        targetCamPos.y += thirdPersonHeight;         // Above ship
        currentFollowLerp = thirdPersonFollowLerp;
    }
    
    // Smooth lerp to target position (creates lag/follow effect)
    float lerpFactor = glm::clamp(currentFollowLerp * deltaTime, 0.0f, 1.0f);
    position = glm::mix(position, targetCamPos, lerpFactor);
}

void CameraController::toggleMode() {
    if (mode == CameraMode::FirstPerson) {
        mode = CameraMode::ThirdPerson;
    } else {
        mode = CameraMode::FirstPerson;
    }
}

glm::mat4 CameraController::getViewMatrix() const {
    // Look from camera position straight ahead along -Z
    return glm::lookAt(position, position + front, up);
}
