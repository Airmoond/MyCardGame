#pragma once

#include <string>
#include <vector>

#include "battle.h"
#include "game_content.h"

// 下面这一组 Bridge* 结构是“给表现层看的数据”。
// 它们会把核心层的枚举和内部结构转成更直接的字符串/扁平数据。
struct BridgeCardEffect {
    std::string type;
    std::string target;
    int value = 0;
};

struct BridgeCardDefinition {
    int defId = -1;
    std::string id;
    std::string name;
    int baseCost = 0;
    std::vector<BridgeCardEffect> effects;
};

struct BridgeCardState {
    int instanceId = -1;
    int defId = -1;
    std::string id;
    std::string name;
    int cost = 0;
    std::vector<BridgeCardEffect> effects;
};

struct BridgeEntityState {
    int hp = 0;
    int maxHp = 0;
    int block = 0;
};

struct BridgeEnemyIntent {
    bool known = false;
    std::string type;
    std::string target;
    int value = 0;
};

struct BridgeBattleSnapshot {
    std::string phase;
    int turn = 0;
    int energy = 0;
    int energyCap = 0;
    int energyMax = 0;
    std::string enemyName;
    BridgeEntityState player;
    BridgeEntityState enemy;
    BridgeEnemyIntent enemyIntent;
    std::vector<BridgeCardState> hand;
    int drawpileCount = 0;
    int discardCount = 0;
    bool hasPendingActions = false;
    bool canAcceptInput = false;
    bool isBattleOver = false;
};

struct BridgeBattleEvent {
    std::string type;
    std::string phase;
    std::string entity;
    int amount = 0;
    int amount2 = 0;
    int turn = 0;
    int cardInstanceId = -1;
    int cardDefId = -1;
    std::string cardId;
    std::string cardName;
    std::string fromZone;
    std::string toZone;
};

// BattleBridge 是 Godot / 前端层的推荐入口。
// 你可以把它理解为“Battle 的 UI 友好包装器”。
class BattleBridge {
public:
    BattleBridge();
    explicit BattleBridge(BattleConfig config);

    // 设置战斗配置和随机种子。
    void setBattleConfig(BattleConfig config);
    void setRandomSeed(unsigned int seed);

    // startDefaultBattle 使用内置默认配置，startBattle 使用当前 config_。
    void startDefaultBattle();
    void startBattle();

    // 前端命令入口。
    PlayResult playCardAtHandIndex(int handIndex);
    PlayResult playCardByInstanceId(int instanceId);
    bool endTurn();
    bool advanceOneStep();
    bool advanceUntilNextDecision();

    // 前端读取入口。
    bool canAcceptInput() const;
    bool isBattleOver() const;
    BridgeBattleSnapshot getSnapshot() const;
    std::vector<BridgeBattleEvent> consumeEvents();
    std::vector<BridgeCardDefinition> getCardCatalog() const;

    // 仅在需要更底层调试时再直接访问 Battle。
    const Battle& getBattle() const;

private:
    // 把核心枚举转成表现层容易消费的 token。
    static std::string toToken(EffectType type);
    static std::string toToken(EntityId id);
    static std::string toToken(CardZone zone);
    static std::string toToken(BattlePhase phase);
    static std::string toToken(BattleEventType type);

    static BridgeCardEffect makeCardEffect(const EffectData& effect);
    BridgeCardDefinition makeCardDefinition(int defId, const CardData& def) const;
    BridgeCardState makeCardState(const CardView& card) const;

    // bridge 内部自己持有一份战斗实例和对应配置。
    Battle battle_;
    BattleConfig config_;
};
