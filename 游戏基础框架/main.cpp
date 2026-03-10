#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "battle.h"

namespace {

std::string describeEffect(const Effect& effect) {
    switch (effect.type) {
    case EffectType::Damage:
        return "对敌人造成" + std::to_string(effect.value) + "点伤害";
    case EffectType::Block:
        return "获得" + std::to_string(effect.value) + "点格挡";
    }

    return "未知效果";
}

std::string describeEffects(const CardDef& def) {
    if (def.effects.empty()) {
        return "无效果";
    }

    std::ostringstream output;
    for (size_t i = 0; i < def.effects.size(); ++i) {
        if (i > 0) {
            output << "，";
        }
        output << describeEffect(def.effects[i]);
    }
    return output.str();
}

std::string describePlayedCardResult(const CardDef& def,
                                     const Entity& playerBefore,
                                     const Entity& enemyBefore,
                                     const Battle& battleAfter) {
    std::ostringstream output;
    output << "你使用了【" << def.name << "】";

    bool firstDetail = true;
    auto appendDetail = [&](const std::string& text) {
        output << (firstDetail ? "，" : "；") << text;
        firstDetail = false;
    };

    for (const Effect& effect : def.effects) {
        if (effect.type == EffectType::Damage) {
            int blocked = std::max(0, enemyBefore.block - battleAfter.enemy.block);
            int actualDamage = std::max(0, enemyBefore.hp - battleAfter.enemy.hp);
            std::ostringstream detail;
            detail << "对敌人造成" << effect.value << "点伤害";
            if (blocked > 0) {
                detail << "（其中" << blocked << "点被格挡吸收";
                if (actualDamage > 0) {
                    detail << "，实际扣除" << actualDamage << "点生命";
                }
                detail << "）";
            } else if (actualDamage != effect.value) {
                detail << "（实际扣除" << actualDamage << "点生命）";
            }
            appendDetail(detail.str());
        } else if (effect.type == EffectType::Block) {
            int gainedBlock = battleAfter.player.block - playerBefore.block;
            appendDetail("获得" + std::to_string(gainedBlock) + "点格挡");
        }
    }

    if (firstDetail) {
        output << "。";
    } else {
        output << "。";
    }

    return output.str();
}

void printStatus(const Battle& battle) {
    std::cout << "\n====================\n";
    std::cout << "回合: " << battle.turn << "\n";
    std::cout << "玩家生命: " << battle.player.hp << "/" << battle.player.maxHp
              << "  格挡: " << battle.player.block
              << "  能量: " << battle.energy << "/" << battle.energyMax << "\n";
    std::cout << "敌人生命: " << battle.enemy.hp << "/" << battle.enemy.maxHp
              << "  格挡: " << battle.enemy.block << "\n";
}

void printHand(const Battle& battle) {
    std::cout << "手牌:\n";
    for (int i = 0; i < static_cast<int>(battle.hand.size()); ++i) {
        const CardInstance& card = battle.hand[i];
        if (card.defId < 0 || card.defId >= static_cast<int>(battle.cardDefs.size())) {
            std::cout << "  [" << i << "] <无效卡牌>\n";
            continue;
        }

        const CardDef& def = battle.cardDefs[card.defId];
        int totalCost = def.baseCost + card.costDelta;
        if (totalCost < 0) {
            totalCost = 0;
        }

        std::cout << "  [" << i << "] " << def.name << "  费用=" << totalCost
                  << "  效果=" << describeEffects(def) << "\n";
    }

    if (battle.hand.empty()) {
        std::cout << "  (空)\n";
    }
}

void printHelp() {
    std::cout << "命令: p <索引> = 出牌, e = 结束回合, q = 退出\n";
}

void printPlayResult(PlayResult result) {
    switch (result) {
    case PlayResult::Ok:
        std::cout << "出牌成功。\n";
        break;
    case PlayResult::InvalidIndex:
        std::cout << "卡牌索引无效。\n";
        break;
    case PlayResult::NotEnoughEnergy:
        std::cout << "能量不足。\n";
        break;
    }
}

}  // namespace

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    Battle battle;
    battle.rng.seed(static_cast<unsigned int>(std::time(nullptr)));
    battle.startBattle();

    std::cout << "简单卡牌战斗演示\n";
    printHelp();

    while (battle.player.hp > 0 && battle.enemy.hp > 0) {
        battle.startTurn();
        std::cout << "\n玩家回合开始。\n";

        bool turnEnded = false;
        while (!turnEnded && battle.player.hp > 0 && battle.enemy.hp > 0) {
            printStatus(battle);
            printHand(battle);
            std::cout << "> ";

            std::string line;
            if (!std::getline(std::cin, line)) {
                std::cout << "\n输入已关闭。\n";
                return 0;
            }

            std::istringstream input(line);
            std::string command;
            input >> command;

            if (command == "p") {
                int handIndex = -1;
                if (!(input >> handIndex)) {
                    std::cout << "用法: p <索引>\n";
                    continue;
                }

                if (handIndex >= 0 && handIndex < static_cast<int>(battle.hand.size())) {
                    const CardInstance& card = battle.hand[handIndex];
                    if (card.defId >= 0 && card.defId < static_cast<int>(battle.cardDefs.size())) {
                        const CardDef& def = battle.cardDefs[card.defId];
                        Entity playerBefore = battle.player;
                        Entity enemyBefore = battle.enemy;
                        PlayResult result = battle.playCard(handIndex);
                        if (result == PlayResult::Ok) {
                            std::cout << describePlayedCardResult(def, playerBefore, enemyBefore, battle) << "\n";
                        } else {
                            printPlayResult(result);
                        }
                        continue;
                    }
                }

                printPlayResult(battle.playCard(handIndex));
            } else if (command == "e") {
                turnEnded = true;
            } else if (command == "q") {
                std::cout << "已退出。\n";
                return 0;
            } else {
                printHelp();
            }
        }

        if (battle.enemy.hp <= 0) {
            break;
        }

        battle.endTurn();
        battle.enemyAct();

        if (battle.player.hp > 0) {
            std::cout << "敌人行动，对你造成6点伤害。\n";
        }
    }

    if (battle.enemy.hp <= 0) {
        std::cout << "\n你赢了。\n";
    } else if (battle.player.hp <= 0) {
        std::cout << "\n你输了。\n";
    }

    battle.endBattle();
    return 0;
}
