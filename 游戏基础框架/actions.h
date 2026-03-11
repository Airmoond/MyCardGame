#pragma once

#include <deque>
#include <memory>

#include "battle_types.h"

class Battle;

class GameAction {
public:
    virtual ~GameAction() = default;
    virtual void execute(Battle& battle) = 0;
};

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

class PlayCardAction : public GameAction {
public:
    explicit PlayCardAction(CardInstance card);
    void execute(Battle& battle) override;

private:
    CardInstance card_;
};

class DamageAction : public GameAction {
public:
    DamageAction(EntityId target, int amount);
    void execute(Battle& battle) override;

private:
    EntityId target_;
    int amount_;
};

class GainBlockAction : public GameAction {
public:
    GainBlockAction(EntityId target, int amount);
    void execute(Battle& battle) override;

private:
    EntityId target_;
    int amount_;
};

class DrawCardsAction : public GameAction {
public:
    explicit DrawCardsAction(int amount);
    void execute(Battle& battle) override;

private:
    int amount_;
};

class EnemyAttackAction : public GameAction {
public:
    explicit EnemyAttackAction(int amount);
    void execute(Battle& battle) override;

private:
    int amount_;
};
