#ifndef BASELEVEL_H
#define BASELEVEL_H

#include <string>
#include <glm/glm.hpp>

// Forward declarations
class Game;
class Shader;
class PlayerShip;

enum class LevelType {
    Level1_OuterDriftZone,
    Level2_SolarDebrisPath
};

class BaseLevel {
public:
    virtual ~BaseLevel() = default;
    
    // Initialize level resources (called once when level starts)
    virtual void init(Game& game) = 0;
    
    // Update level logic each frame
    virtual void update(float dt, Game& game) = 0;
    
    // Render level-specific geometry
    virtual void render(Game& game) = 0;
    
    // Check if level objectives are completed
    virtual bool isCompleted() const = 0;
    
    // Get level name for display
    virtual std::string getLevelName() const = 0;
    
    // Get level type
    virtual LevelType getLevelType() const = 0;
    
    // Cleanup resources (called when switching levels)
    virtual void cleanup() = 0;
    
    // Get player spawn position for this level
    virtual glm::vec3 getPlayerSpawnPosition() const = 0;
    virtual glm::vec3 getPlayerSpawnRotation() const = 0;
    
    // Score persistence between levels
    virtual int getScore() const = 0;
    virtual void setScore(int newScore) = 0;
};

#endif
