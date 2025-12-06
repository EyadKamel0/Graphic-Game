#ifndef LEVELMANAGER_H
#define LEVELMANAGER_H

#include "BaseLevel.h"
#include <memory>
#include <vector>

class Game;

class LevelManager {
public:
    LevelManager();
    ~LevelManager();
    
    // Initialize level manager with starting level
    void init(Game& game);
    
    // Update current level
    void update(float dt, Game& game);
    
    // Render current level
    void render(Game& game);
    
    // Get current level
    BaseLevel* getCurrentLevel() const;
    
    // Check if all levels are completed
    bool isGameComplete() const;
    
    // Force switch to next level (for testing or progression)
    void switchToNextLevel(Game& game);
    
    // Skip directly to a specific level (for testing)
    void skipToLevel(int levelIndex, Game& game);
    
    // Get current level index
    int getCurrentLevelIndex() const;
    
    // Get total number of levels
    int getTotalLevels() const;
    
    // Fire a bullet in the current level (if supported)
    void fireBulletInCurrentLevel(const glm::vec3& position, const glm::vec3& direction);
    
    // Check if player controls should be disabled (e.g., during win screen)
    bool areControlsDisabled() const;
    
    // Check if rapid fire power-up is active in current level
    bool isRapidFireActive() const;
    
private:
    std::vector<std::unique_ptr<BaseLevel>> levels;
    int currentLevelIndex;
    bool waitingForLevelSwitch;
    
    void loadLevel(int index, Game& game);
};

#endif
