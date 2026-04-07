#pragma once

#include <memory>
#include <random>
#include <vector>

#include "actions.h"

// Battle 是战斗核心。
// 它只关心规则推进、状态变化和事件记录，不关心控制台或 Godot 具体怎么显示。
class Battle {
public:
    // 用一份 BattleConfig 初始化一场新战斗。
    void initialize(const BattleConfig& config);

    // 给随机数生成器设种子，方便复现战斗过程或做调试。
    void setRandomSeed(unsigned int seed);

    // 将战斗推进一步。
    // 这一步可能是开始回合、开始敌人行动，或执行动作队列中的一个动作。
    bool advance();

    // 按手牌索引出牌。更适合演示前端使用。
    PlayResult tryPlayCard(int handIndex);

    // 按稳定实例 ID 出牌。Godot 侧更推荐使用这个接口。
    PlayResult tryPlayCardByInstanceId(int instanceId);

    // 结束玩家回合。
    void endPlayerTurn();

    // 一次性把动作队列跑完。更适合测试或同步型前端。
    void resolveActions();

    // 是否允许玩家当前输入命令。
    bool canAcceptInput() const;
    bool isBattleOver() const;
    bool hasPendingActions() const;

    // 读取各种只读状态。
    int computeCardCost(const CardInstance& card) const;
    int getTurn() const;
    BattlePhase getPhase() const;
    const Entity& getEntity(EntityId id) const;
    const EnemyData& getEnemyData() const;
    const std::vector<CardData>& getCardDefs() const;
    const CardData* tryGetCardDef(int defId) const;
    const std::vector<CardInstance>& getHand() const;
    BattleSnapshot getSnapshot() const;
    EnemyIntent getEnemyIntent() const;

    // 取出并清空当前累计的事件列表。
    std::vector<BattleEvent> consumeEvents();

    // 下面这些接口主要给动作层和内部流程调用。
    int drawCards(int n);
    void discardHand();

    void queueAction(std::unique_ptr<GameAction> action);
    void expandCardEffects(const CardInstance& card);
    void enqueueEffect(const EffectData& effect);
    void updateOutcome();

    void applyDamage(EntityId target, int damage);
    void gainBlock(EntityId target, int block);

private:
    // 统一设置 phase，并记录阶段变化事件。
    void setPhase(BattlePhase phase);
    void recordEvent(BattleEvent event);
    void recordEnergyChanged();
    void recordBlockChanged(EntityId id, int delta, int newBlock);

    // 这几个是内部阶段推进函数。
    void beginPlayerTurn();
    void queueEnemyTurn();
    void startPlayerTurn();
    void startEnemyTurn();
    void onActionQueueDrained();

    // 下面几个是运行时辅助函数。
    CardView makeCardView(const CardInstance& card) const;
    int findHandIndexByInstanceId(int instanceId) const;
    void assignCardInstanceIds(std::vector<CardInstance>& cards);
    void reshuffleIfNeeded();

    // 运行时战斗状态。
    BattlePhase phase_ = BattlePhase::BattleStart;
    Entity player_;
    Entity enemy_;
    int turn_ = 0;
    int energy_ = 0;
    int energyCap_ = 0;
    int energyMax_ = 0;
    int openingHandNum_ = 0;
    int drawPerTurn_ = 0;
    int handLimit_ = 0;
    int nextCardInstanceId_ = 1;
    EnemyData enemyData_;
    std::vector<CardData> cardDefs_;
    std::vector<CardInstance> hand_;
    std::vector<CardInstance> drawpile_;
    std::vector<CardInstance> discard_;
    std::vector<BattleEvent> pendingEvents_;
    std::mt19937 rng;
    ActionQueue actionQueue_;
};
