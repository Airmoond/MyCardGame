#include "battle_runner.h"

#include <utility>

BattleRunner::BattleRunner(Battle& battle, BattleConfig config)
    : battle_(battle), config_(std::move(config)) {
}

void BattleRunner::startBattle() {
    // startBattle 会把战斗初始化，然后一口气推进到下一个可输入节点。
    battle_.initialize(config_);
    advanceToNextDecision();
}

PlayResult BattleRunner::playCard(int handIndex) {
    const PlayResult result = battle_.tryPlayCard(handIndex);
    advanceToNextDecision();
    return result;
}

void BattleRunner::endPlayerTurn() {
    battle_.endPlayerTurn();
    advanceToNextDecision();
}

void BattleRunner::advanceToNextDecision() {
    // Runner 仍然保留“自动推进到下一个玩家决策点”的便利模式，
    // 但 Battle 本体已经支持逐步推进，适合后续接 Godot 做异步表现。
    while (!battle_.isBattleOver() && !battle_.canAcceptInput()) {
        if (!battle_.advance()) {
            break;
        }
    }
}
