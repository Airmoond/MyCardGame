#include "game_content.h"

#include <vector>

BattleConfig createDefaultBattleConfig() {
    BattleConfig config;
    config.playerMaxHp = 50;
    config.startingEnergy = 2;
    config.energyCap = 5;
    config.openingHandNum = 4;
    config.drawPerTurn = 2;
    config.handLimit = 10;
    config.enemy = { "训练木桩", 40, 6 };

    config.cardDefs = {
        { "strike", "打击", 1, { { EffectType::Damage, 6, EntityId::Enemy } } },
        { "defend", "防御", 1, { { EffectType::Block, 5, EntityId::Player } } },
    };

    // 具体游戏内容只在这一层配置，框架层只消费 BattleConfig。
    for (int i = 0; i < 10; ++i) {
        config.startingDeck.push_back({ 0, 0 });
        config.startingDeck.push_back({ 1, 0 });
    }

    return config;
}
