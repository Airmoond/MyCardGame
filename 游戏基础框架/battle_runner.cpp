#include "battle_runner.h"

#include <utility>

BattleRunner::BattleRunner(Battle& battle, BattleConfig config)
    : battle_(battle), config_(std::move(config)) {
}

void BattleRunner::startBattle() {
    battle_.initialize(config_);
    update();
}

PlayResult BattleRunner::playCard(int handIndex) {
    const PlayResult result = battle_.tryPlayCard(handIndex);
    update();
    return result;
}

void BattleRunner::endPlayerTurn() {
    battle_.endPlayerTurn();
    update();
}

void BattleRunner::update() {
    // Runner 负责推进非交互阶段，main 只保留输入输出。
    while (!battle_.isBattleOver()) {
        if (battle_.phase == BattlePhase::BattleStart ||
            battle_.phase == BattlePhase::PlayerTurnStart) {
            battle_.beginPlayerTurn();
            continue;
        }

        if (battle_.phase == BattlePhase::EnemyTurnStart) {
            battle_.queueEnemyTurn();
            battle_.resolveActions();
            if (!battle_.isBattleOver()) {
                battle_.phase = BattlePhase::PlayerTurnStart;
            }
            continue;
        }

        break;
    }
}
