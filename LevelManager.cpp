#include "LevelManager.h"
#include "Game.h"
#include "Level1OuterDriftZone.h"
#include "Level2SolarDebrisPath.h"
#include "PlayerShip.h"
#include <iostream>

LevelManager::LevelManager()
    : currentLevelIndex(0), waitingForLevelSwitch(false) {
}

LevelManager::~LevelManager() {
    // Clean up current level
    if (currentLevelIndex >= 0 && currentLevelIndex < levels.size()) {
        levels[currentLevelIndex]->cleanup();
    }
}

void LevelManager::init(Game& game) {
    // Create all levels
    levels.push_back(std::make_unique<Level1OuterDriftZone>());
    levels.push_back(std::make_unique<Level2SolarDebrisPath>());
    
    // Start at level 0 but don't load yet (will be loaded after intro)
    currentLevelIndex = 0;
    waitingForLevelSwitch = false;
    
    std::cout << "Level Manager initialized with " << levels.size() << " levels" << std::endl;
}

void LevelManager::update(float dt, Game& game) {
    if (currentLevelIndex < 0 || currentLevelIndex >= levels.size()) {
        return;
    }
    
    BaseLevel* currentLevel = levels[currentLevelIndex].get();
    
    // Update current level
    currentLevel->update(dt, game);
    
    // Check for level completion
    if (currentLevel->isCompleted() && !waitingForLevelSwitch) {
        waitingForLevelSwitch = true;
        
        if (currentLevelIndex < levels.size() - 1) {
            std::cout << "\n========================================" << std::endl;
            std::cout << currentLevel->getLevelName() << " completed!" << std::endl;
            std::cout << "Entering next level..." << std::endl;
            std::cout << "========================================\n" << std::endl;
            
            // Auto-switch to next level immediately
            switchToNextLevel(game);
        } else {
            std::cout << "\n========================================" << std::endl;
            std::cout << "ALL LEVELS COMPLETED!" << std::endl;
            std::cout << "Congratulations, you finished Starwave 3D!" << std::endl;
            std::cout << "========================================\n" << std::endl;
        }
    }
}

void LevelManager::render(Game& game) {
    if (currentLevelIndex >= 0 && currentLevelIndex < levels.size()) {
        levels[currentLevelIndex]->render(game);
    }
}

BaseLevel* LevelManager::getCurrentLevel() const {
    if (currentLevelIndex >= 0 && currentLevelIndex < levels.size()) {
        return levels[currentLevelIndex].get();
    }
    return nullptr;
}

bool LevelManager::isGameComplete() const {
    return currentLevelIndex >= levels.size() - 1 && 
           levels[currentLevelIndex]->isCompleted();
}

void LevelManager::switchToNextLevel(Game& game) {
    if (currentLevelIndex < levels.size() - 1) {
        // Save score from current level before cleanup
        int savedScore = levels[currentLevelIndex]->getScore();
        
        // Cleanup current level
        levels[currentLevelIndex]->cleanup();
        
        // Move to next level
        currentLevelIndex++;
        loadLevel(currentLevelIndex, game);
        
        // Transfer score to new level
        levels[currentLevelIndex]->setScore(savedScore);
        std::cout << "[LEVEL] Score transferred: " << savedScore << std::endl;
        
        waitingForLevelSwitch = false;
        
        std::cout << "\nSwitching to " << levels[currentLevelIndex]->getLevelName() << "...\n" << std::endl;
    } else {
        std::cout << "Already on the last level!" << std::endl;
    }
}

void LevelManager::skipToLevel(int levelIndex, Game& game) {
    if (levelIndex < 0 || levelIndex >= levels.size()) {
        std::cerr << "Invalid level index: " << levelIndex << std::endl;
        return;
    }
    
    if (levelIndex == currentLevelIndex) {
        std::cout << "Already on level " << (levelIndex + 1) << std::endl;
        return;
    }
    
    // Cleanup current level
    levels[currentLevelIndex]->cleanup();
    
    // Jump to specified level
    currentLevelIndex = levelIndex;
    loadLevel(currentLevelIndex, game);
    waitingForLevelSwitch = false;
    
    std::cout << "\n[DEBUG] Jumped to " << levels[currentLevelIndex]->getLevelName() << "\n" << std::endl;
}

void LevelManager::resetToFirstLevel(Game& game) {
    // Always cleanup current level
    levels[currentLevelIndex]->cleanup();
    
    // HARDCODED: Force reset to first level (Level 1 = index 0)
    currentLevelIndex = 0;
    waitingForLevelSwitch = false;
    
    // Load Level 1
    loadLevel(0, game);
    
    // CRITICAL: Mark Level 2 as not completed to prevent auto-switch
    if (levels.size() > 1) {
        levels[1]->resetCompletionState();
    }
    
    std::cout << "\n[RESET] Forced reset to Level 1: " << levels[0]->getLevelName() << "\n" << std::endl;
}

int LevelManager::getCurrentLevelIndex() const {
    return currentLevelIndex;
}

int LevelManager::getTotalLevels() const {
    return levels.size();
}

void LevelManager::loadLevel(int index, Game& game) {
    if (index < 0 || index >= levels.size()) {
        std::cerr << "Invalid level index: " << index << std::endl;
        return;
    }
    
    BaseLevel* level = levels[index].get();
    
    // Initialize level
    level->init(game);
    
    // Respawn player at level's spawn point (on-rails mode: only set X/Y position)
    PlayerShip* playerShip = game.getPlayerShip();
    if (playerShip) {
        glm::vec3 spawnPos = level->getPlayerSpawnPosition();
        playerShip->position = spawnPos;
        playerShip->position.z = playerShip->fixedZ;  // Enforce fixed Z for on-rails
        playerShip->velocity = glm::vec3(0.0f);
        playerShip->bankAngle = 0.0f;  // Reset visual banking
    }
    
    std::cout << "Loaded: " << level->getLevelName() << std::endl;
    std::cout << "Objective: (Press ENTER when ready to progress)" << std::endl;
}

void LevelManager::fireBulletInCurrentLevel(const glm::vec3& position, const glm::vec3& direction) {
    if (currentLevelIndex < 0 || currentLevelIndex >= static_cast<int>(levels.size())) {
        return;
    }
    
    BaseLevel* level = levels[currentLevelIndex].get();
    if (!level) return;
    
    // Try to cast to Level1OuterDriftZone (which has shooting)
    Level1OuterDriftZone* level1 = dynamic_cast<Level1OuterDriftZone*>(level);
    if (level1) {
        level1->fireBullet(position, direction);
        return;
    }
    
    // Try to cast to Level2SolarDebrisPath (which also has shooting)
    Level2SolarDebrisPath* level2 = dynamic_cast<Level2SolarDebrisPath*>(level);
    if (level2) {
        level2->fireBullet(position, direction);
        return;
    }
}

bool LevelManager::areControlsDisabled() const {
    if (currentLevelIndex < 0 || currentLevelIndex >= static_cast<int>(levels.size())) {
        return false;
    }
    
    BaseLevel* level = levels[currentLevelIndex].get();
    if (!level) return false;
    
    // Check if Level2 has controls disabled (during black hole freeze)
    Level2SolarDebrisPath* level2 = dynamic_cast<Level2SolarDebrisPath*>(level);
    if (level2) {
        return level2->isControlsDisabled();
    }
    
    return false;
}

bool LevelManager::isRapidFireActive() const {
    if (currentLevelIndex < 0 || currentLevelIndex >= static_cast<int>(levels.size())) {
        return false;
    }
    
    BaseLevel* level = levels[currentLevelIndex].get();
    if (!level) return false;
    
    // Check if Level2 has rapid fire active
    Level2SolarDebrisPath* level2 = dynamic_cast<Level2SolarDebrisPath*>(level);
    if (level2) {
        return level2->isRapidFireActive();
    }
    
    return false;
}
