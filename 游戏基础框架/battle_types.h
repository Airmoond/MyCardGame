#pragma once

#include <string>
#include <vector>

enum class EffectType {
    Damage,
    Block,
    Draw,
};

enum class EntityId {
    Player,
    Enemy,
};

enum class BattlePhase {
    BattleStart,
    PlayerTurnStart,
    PlayerInput,
    EnemyTurnStart,
    EnemyActing,
    Victory,
    Defeat,
};

enum class PlayResult {
    Ok,
    InvalidIndex,
    NotEnoughEnergy,
    InvalidPhase,
};

struct EffectData {
    EffectType type = EffectType::Damage;
    int value = 0;
    EntityId target = EntityId::Enemy;
};

struct CardData {
    std::string id;
    std::string name;
    int baseCost = 0;
    std::vector<EffectData> effects;
};

struct CardInstance {
    int defId = -1;
    int costDelta = 0;
};

struct Entity {
    int hp = 0;
    int maxHp = 0;
    int block = 0;
};

struct EnemyData {
    std::string name;
    int maxHp = 0;
    int attackDamage = 0;
};

struct BattleConfig {
    int playerMaxHp = 50;
    int startingEnergy = 2;
    int energyCap = 5;
    int openingHandNum = 4;
    int drawPerTurn = 2;
    int handLimit = 10;
    EnemyData enemy;
    std::vector<CardData> cardDefs;
    std::vector<CardInstance> startingDeck;
};
