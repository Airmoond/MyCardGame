#pragma once

#include "battle.h"

class BattleRunner {
public:
    BattleRunner(Battle& battle, BattleConfig config);

    void startBattle();
    PlayResult playCard(int handIndex);
    void endPlayerTurn();

private:
    void update();

    Battle& battle_;
    BattleConfig config_;
};
