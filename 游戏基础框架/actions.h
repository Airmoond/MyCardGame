#pragma once

#include <deque>
#include <memory>

#include "battle_types.h"

class Battle;

// 所有战斗动作的抽象基类。
// 规则层通过动作队列把“玩家命令”拆成更细的执行步骤。
class GameAction {
public:
    virtual ~GameAction() = default;
    virtual void execute(Battle& battle) = 0;
};

// 动作队列负责串行执行战斗动作。
// 队列中的动作在执行期间还可以继续向队列尾部追加新动作。
class ActionQueue {
public:
    void push(std::unique_ptr<GameAction> action);
    bool empty() const;
    void clear();
    void executeNext(Battle& battle);
    void executeAll(Battle& battle);

private:
    std::deque<std::unique_ptr<GameAction>> actions_;
};

// “打出一张牌”的高层动作。
// 它本身不直接结算数值，而是展开这张牌上的效果列表。
class PlayCardAction : public GameAction {
public:
    explicit PlayCardAction(CardInstance card);
    void execute(Battle& battle) override;

private:
    CardInstance card_;
};

// 直接伤害动作。
class DamageAction : public GameAction {
public:
    DamageAction(EntityId target, int amount);
    void execute(Battle& battle) override;

private:
    EntityId target_;
    int amount_;
};

// 获得格挡动作。
class GainBlockAction : public GameAction {
public:
    GainBlockAction(EntityId target, int amount);
    void execute(Battle& battle) override;

private:
    EntityId target_;
    int amount_;
};

// 抽牌动作。
class DrawCardsAction : public GameAction {
public:
    explicit DrawCardsAction(int amount);
    void execute(Battle& battle) override;

private:
    int amount_;
};

// 敌人的一次基础攻击动作。
// 当前实现会把它进一步展开为对玩家造成伤害的基础动作。
class EnemyAttackAction : public GameAction {
public:
    explicit EnemyAttackAction(int amount);
    void execute(Battle& battle) override;

private:
    int amount_;
};
