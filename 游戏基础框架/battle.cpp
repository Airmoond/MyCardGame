#include <algorithm>
#include <memory>

#include "battle.h"

void Battle::initialize(const BattleConfig& config) {
    phase = BattlePhase::BattleStart;
    turn = 0;
    energyCap = config.energyCap;
    energyMax = config.startingEnergy;
    energy = energyMax;
    openingHandNum = config.openingHandNum;
    drawPerTurn = config.drawPerTurn;
    handLimit = config.handLimit;
    enemyData = config.enemy;

    player = { config.playerMaxHp, config.playerMaxHp, 0 };
    enemy = { enemyData.maxHp, enemyData.maxHp, 0 };

    cardDefs = config.cardDefs;
    hand.clear();
    discard.clear();
    drawpile = config.startingDeck;
    actionQueue_.clear();

    std::shuffle(drawpile.begin(), drawpile.end(), rng);
    drawCards(openingHandNum);
    updateOutcome();
}

void Battle::beginPlayerTurn() {
    if (isBattleOver()) {
        return;
    }

    phase = BattlePhase::PlayerTurnStart;
    ++turn;
    player.block = 0;

    if (energyMax < energyCap && turn > 1) {
        ++energyMax;
    }

    energy = energyMax;
    if (turn > 1) {
        drawCards(drawPerTurn);
    }

    phase = BattlePhase::PlayerInput;
    updateOutcome();
}

PlayResult Battle::tryPlayCard(int handIndex) {
    if (!canAcceptInput()) {
        return PlayResult::InvalidPhase;
    }

    if (handIndex < 0 || handIndex >= static_cast<int>(hand.size())) {
        return PlayResult::InvalidIndex;
    }

    const CardInstance card = hand[handIndex];
    if (card.defId < 0 || card.defId >= static_cast<int>(cardDefs.size())) {
        return PlayResult::InvalidIndex;
    }

    const int totalCost = computeCardCost(card);
    if (energy < totalCost) {
        return PlayResult::NotEnoughEnergy;
    }

    energy -= totalCost;
    discard.push_back(card);
    hand.erase(hand.begin() + handIndex);

    queueAction(std::make_unique<PlayCardAction>(card));
    resolveActions();

    return PlayResult::Ok;
}

void Battle::endPlayerTurn() {
    if (!canAcceptInput()) {
        return;
    }

    discardHand();
    phase = BattlePhase::EnemyTurnStart;
    updateOutcome();
}

void Battle::queueEnemyTurn() {
    if (isBattleOver()) {
        return;
    }

    phase = BattlePhase::EnemyActing;
    enemy.block = 0;
    queueAction(std::make_unique<EnemyAttackAction>(enemyData.attackDamage));
}

void Battle::resolveActions() {
    actionQueue_.executeAll(*this);
    updateOutcome();
}

bool Battle::canAcceptInput() const {
    return phase == BattlePhase::PlayerInput && !isBattleOver();
}

bool Battle::isBattleOver() const {
    return phase == BattlePhase::Victory || phase == BattlePhase::Defeat;
}

int Battle::computeCardCost(const CardInstance& card) const {
    if (card.defId < 0 || card.defId >= static_cast<int>(cardDefs.size())) {
        return 0;
    }

    const CardData& def = cardDefs[card.defId];
    return std::max(0, def.baseCost + card.costDelta);
}

Entity& Battle::getEntity(EntityId id) {
    return id == EntityId::Player ? player : enemy;
}

const Entity& Battle::getEntity(EntityId id) const {
    return id == EntityId::Player ? player : enemy;
}

int Battle::drawCards(int n) {
    int actualDrawn = 0;
    for (int i = 0; i < n; ++i) {
        if (hand.size() >= static_cast<size_t>(handLimit)) {
            break;
        }

        reshuffleIfNeeded();
        if (drawpile.empty()) {
            break;
        }

        hand.push_back(drawpile.back());
        drawpile.pop_back();
        ++actualDrawn;
    }

    return actualDrawn;
}

void Battle::discardHand() {
    for (const CardInstance& card : hand) {
        discard.push_back(card);
    }
    hand.clear();
}

void Battle::queueAction(std::unique_ptr<GameAction> action) {
    actionQueue_.push(std::move(action));
}

void Battle::expandCardEffects(const CardInstance& card) {
    if (card.defId < 0 || card.defId >= static_cast<int>(cardDefs.size())) {
        return;
    }

    const CardData& cardDef = cardDefs[card.defId];
    for (const EffectData& effect : cardDef.effects) {
        enqueueEffect(effect);
    }
}

void Battle::enqueueEffect(const EffectData& effect) {
    switch (effect.type) {
    case EffectType::Damage:
        queueAction(std::make_unique<DamageAction>(effect.target, effect.value));
        break;
    case EffectType::Block:
        queueAction(std::make_unique<GainBlockAction>(effect.target, effect.value));
        break;
    case EffectType::Draw:
        queueAction(std::make_unique<DrawCardsAction>(effect.value));
        break;
    }
}

void Battle::updateOutcome() {
    if (enemy.hp <= 0) {
        enemy.hp = 0;
        phase = BattlePhase::Victory;
    } else if (player.hp <= 0) {
        player.hp = 0;
        phase = BattlePhase::Defeat;
    }
}

void Battle::applyDamage(Entity& target, int damage) {
    const int damageAfterBlock = std::max(0, damage - target.block);
    target.block = std::max(0, target.block - damage);
    target.hp = std::max(0, target.hp - damageAfterBlock);
}

void Battle::gainBlock(Entity& target, int block) {
    target.block += block;
}

void Battle::reshuffleIfNeeded() {
    if (!drawpile.empty() || discard.empty()) {
        return;
    }

    drawpile = discard;
    discard.clear();
    std::shuffle(drawpile.begin(), drawpile.end(), rng);
}
