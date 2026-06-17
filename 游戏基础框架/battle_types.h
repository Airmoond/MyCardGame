#pragma once

#include <string>
#include <vector>

// 基础效果类型。当前只实现了伤害、格挡和抽牌三类最小效果。
enum class EffectType {
    Damage,
    Block,
    Draw,
};

// 战斗中当前能被指向的实体。
enum class EntityId {
    Player,
    Enemy,
};

// 卡牌所在区域。表现层通常依靠这个信息播放移动动画。
enum class CardZone {
    None,
    DrawPile,
    Hand,
    Discard,
};

// 战斗阶段。Godot 接入时通常根据 phase 判断当前是否允许玩家操作。
enum class BattlePhase {
    BattleStart,
    PlayerTurnStart,
    PlayerInput,
    PlayerActing,
    EnemyTurnStart,
    EnemyActing,
    Victory,
    Defeat,
};

// 玩家发出“出牌”命令后的基础返回结果。
enum class PlayResult {
    Ok,
    InvalidIndex,
    NotEnoughEnergy,
    InvalidPhase,
};

// 一条静态效果定义。它只描述“要做什么”，不直接保存运行时状态。
struct EffectData {
    EffectType type = EffectType::Damage;
    int value = 0;
    EntityId target = EntityId::Enemy;
};

// 卡牌原型定义。defId 实际上是它在 cardDefs 数组中的索引。
struct CardData {
    std::string id;
    std::string name;
    int baseCost = 0;
    std::vector<EffectData> effects;
};

// 运行时卡牌实例。instanceId 用来给表现层稳定追踪“这一张牌是谁”。
struct CardInstance {
    int instanceId = -1;
    int defId = -1;
    int costDelta = 0;
};

// 战斗实体的最小运行时状态。
struct Entity {
    int hp = 0;
    int maxHp = 0;
    int block = 0;
};

// 敌人的静态配置。当前只实现了名字、生命和固定攻击伤害。
struct EnemyData {
    std::string name;
    int maxHp = 0;
    int attackDamage = 0;
};

// 战斗核心向表现层输出的事件类型。
// 这层不是“完整回放协议”，但足够支撑当前 demo 的 UI 更新和动画播放。
enum class BattleEventType {
    BattleInitialized,
    PhaseChanged,
    TurnStarted,
    EnergyChanged,
    CardMoved,
    DeckShuffled,
    DamageApplied,
    BlockChanged,
    BattleEnded,
};

// 敌人意图的最小描述。现在只返回一个简单的固定意图。
struct EnemyIntent {
    bool known = false;
    EffectType type = EffectType::Damage;
    EntityId target = EntityId::Player;
    int value = 0;
};

// 手牌展示用的卡牌快照。它是对 CardInstance 的 UI 视图裁剪。
struct CardView {
    int instanceId = -1;
    int defId = -1;
    int cost = 0;
};

// 表现层读取战斗状态时使用的快照。
// 它是“当前时刻的完整截面”，适合直接刷新 UI。
struct BattleSnapshot {
    BattlePhase phase = BattlePhase::BattleStart;
    Entity player;
    Entity enemy;
    int turn = 0;
    int energy = 0;
    int energyCap = 0;
    int energyMax = 0;
    EnemyData enemyData;
    EnemyIntent enemyIntent;
    std::vector<CardView> hand;
    int drawpileCount = 0;
    int discardCount = 0;
    bool hasPendingActions = false;
};

// 表现层消费的单条战斗事件。
// 它更适合做“发生了什么”的动画驱动，而不是完整状态读取。
struct BattleEvent {
    BattleEventType type = BattleEventType::BattleInitialized;
    BattlePhase phase = BattlePhase::BattleStart;
    EntityId entity = EntityId::Player;
    int amount = 0;
    int amount2 = 0;
    int turn = 0;
    int cardInstanceId = -1;
    int cardDefId = -1;
    CardZone fromZone = CardZone::None;
    CardZone toZone = CardZone::None;
};

// 战斗配置。它把规则参数、敌人数据和起始卡组打包成一份开局输入。
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
