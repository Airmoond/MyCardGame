#pragma once

#include "battle.h"

// BattleRunner 是一个“自动推进到下一个玩家决策点”的便利壳。
// 它更适合控制台 demo 或同步前端，不是 Godot 的必需入口。
class BattleRunner {
public:
    BattleRunner(Battle& battle, BattleConfig config);

    void startBattle();
    PlayResult playCard(int handIndex);
    void endPlayerTurn();
    void advanceToNextDecision();

private:
    Battle& battle_;
    BattleConfig config_;
};
