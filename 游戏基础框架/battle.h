#pragma once

#include <memory>
#include <random>
#include <vector>

#include "actions.h"

class Battle {
public:
    BattlePhase phase = BattlePhase::BattleStart;
    Entity player;
    Entity enemy;
    int turn = 0;
    int energy = 0;
    int energyCap = 0;
    int energyMax = 0;
    int openingHandNum = 0;
    int drawPerTurn = 0;
    int handLimit = 0;
    EnemyData enemyData;
    std::vector<CardData> cardDefs;
    std::vector<CardInstance> hand;
    std::vector<CardInstance> drawpile;
    std::vector<CardInstance> discard;
    std::mt19937 rng;

    void initialize(const BattleConfig& config);
    void beginPlayerTurn();
    PlayResult tryPlayCard(int handIndex);
    void endPlayerTurn();
    void queueEnemyTurn();
    void resolveActions();

    bool canAcceptInput() const;
    bool isBattleOver() const;
    int computeCardCost(const CardInstance& card) const;
    Entity& getEntity(EntityId id);
    const Entity& getEntity(EntityId id) const;
    int drawCards(int n);
    void discardHand();

    void queueAction(std::unique_ptr<GameAction> action);
    void expandCardEffects(const CardInstance& card);
    void enqueueEffect(const EffectData& effect);
    void updateOutcome();

    void applyDamage(Entity& target, int damage);
    void gainBlock(Entity& target, int block);

private:
    void reshuffleIfNeeded();

    ActionQueue actionQueue_;
};
