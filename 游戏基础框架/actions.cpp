#include "actions.h"

#include <utility>

#include "battle.h"

void ActionQueue::push(std::unique_ptr<GameAction> action) {
    actions_.push_back(std::move(action));
}

bool ActionQueue::empty() const {
    return actions_.empty();
}

void ActionQueue::clear() {
    actions_.clear();
}

void ActionQueue::executeNext(Battle& battle) {
    if (actions_.empty()) {
        return;
    }

    std::unique_ptr<GameAction> action = std::move(actions_.front());
    actions_.pop_front();
    action->execute(battle);
}

void ActionQueue::executeAll(Battle& battle) {
    // 动作在执行时允许继续向队列追加后续动作，因此要一直跑到队列清空。
    while (!actions_.empty() && !battle.isBattleOver()) {
        executeNext(battle);
        battle.updateOutcome();
    }
}

PlayCardAction::PlayCardAction(CardInstance card) : card_(card) {
}

void PlayCardAction::execute(Battle& battle) {
    battle.expandCardEffects(card_);
}

DamageAction::DamageAction(EntityId target, int amount)
    : target_(target), amount_(amount) {
}

void DamageAction::execute(Battle& battle) {
    battle.applyDamage(battle.getEntity(target_), amount_);
}

GainBlockAction::GainBlockAction(EntityId target, int amount)
    : target_(target), amount_(amount) {
}

void GainBlockAction::execute(Battle& battle) {
    battle.gainBlock(battle.getEntity(target_), amount_);
}

DrawCardsAction::DrawCardsAction(int amount) : amount_(amount) {
}

void DrawCardsAction::execute(Battle& battle) {
    battle.drawCards(amount_);
}

EnemyAttackAction::EnemyAttackAction(int amount) : amount_(amount) {
}

void EnemyAttackAction::execute(Battle& battle) {
    // 敌人动作自身不直接改数值，而是展开成基础动作，保留动作链扩展点。
    battle.queueAction(std::make_unique<DamageAction>(EntityId::Player, amount_));
}
