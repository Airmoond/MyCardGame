#include <algorithm>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "battle_bridge.h"

namespace {

// main.cpp 只是控制台演示前端。
// 它的职责是把 Bridge 返回的数据打印出来，帮助快速观察战斗流程。
std::string describeTarget(const std::string& target) {
    if (target == "player") {
        return "自己";
    }
    if (target == "enemy") {
        return "敌人";
    }

    return "未知目标";
}

std::string describeEffect(const BridgeCardEffect& effect) {
    if (effect.type == "damage") {
        return "对" + describeTarget(effect.target) + "造成" + std::to_string(effect.value) + "点伤害";
    }
    if (effect.type == "block") {
        return describeTarget(effect.target) + "获得" + std::to_string(effect.value) + "点格挡";
    }
    if (effect.type == "draw") {
        return "抽" + std::to_string(effect.value) + "张牌";
    }

    return "未知效果";
}

std::string describeEffects(const std::vector<BridgeCardEffect>& effects) {
    if (effects.empty()) {
        return "无效果";
    }

    std::ostringstream output;
    for (size_t i = 0; i < effects.size(); ++i) {
        if (i > 0) {
            output << "，";
        }
        output << describeEffect(effects[i]);
    }
    return output.str();
}

std::string describeEnemyIntent(const BridgeEnemyIntent& intent) {
    if (!intent.known) {
        return "未知";
    }

    if (intent.type == "damage" && intent.target == "player") {
        return "对玩家造成" + std::to_string(intent.value) + "点伤害";
    }
    if (intent.type == "block") {
        return "获得" + std::to_string(intent.value) + "点格挡";
    }
    if (intent.type == "draw") {
        return "抽" + std::to_string(intent.value) + "张牌";
    }

    return "未知";
}

std::string describePlayedCardResult(const BridgeCardState& card,
                                     const BridgeBattleSnapshot& before,
                                     const BridgeBattleSnapshot& after) {
    std::ostringstream output;
    output << "你使用了【" << card.name << "】";

    bool firstDetail = true;
    auto appendDetail = [&](const std::string& text) {
        output << (firstDetail ? "，" : "；") << text;
        firstDetail = false;
    };

    for (const BridgeCardEffect& effect : card.effects) {
        // 当前控制台文案是通过前后快照推导结果，而不是直接照搬底层事件。
        if (effect.type == "damage" && effect.target == "enemy") {
            const int blocked = std::max(0, before.enemy.block - after.enemy.block);
            const int actualDamage = std::max(0, before.enemy.hp - after.enemy.hp);
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
        } else if (effect.type == "block" && effect.target == "player") {
            const int gainedBlock = after.player.block - before.player.block;
            appendDetail("获得" + std::to_string(gainedBlock) + "点格挡");
        } else if (effect.type == "draw") {
            const int drawn = static_cast<int>(after.hand.size()) - static_cast<int>(before.hand.size()) + 1;
            appendDetail("抽了" + std::to_string(std::max(0, drawn)) + "张牌");
        } else {
            appendDetail(describeEffect(effect));
        }
    }

    output << "。";
    return output.str();
}

void printEnemyTurnResult(const std::vector<BridgeBattleEvent>& events) {
    int totalDamage = 0;
    int totalBlocked = 0;
    bool enemyActed = false;

    for (const BridgeBattleEvent& event : events) {
        if (event.phase == "enemy_acting") {
            enemyActed = true;
        }

        if (event.type == "damage_applied" && event.entity == "player") {
            totalDamage += event.amount;
            totalBlocked += event.amount2;
        }
    }

    if (!enemyActed) {
        return;
    }

    std::cout << "敌人行动";
    if (totalDamage > 0 || totalBlocked > 0) {
        std::cout << "，对你造成" << totalDamage << "点伤害";
        if (totalBlocked > 0) {
            std::cout << "（另有" << totalBlocked << "点被格挡吸收）";
        }
    }
    std::cout << "。\n";
}

void printStatus(const BridgeBattleSnapshot& snapshot) {
    std::cout << "\n====================\n";
    std::cout << "回合: " << snapshot.turn << "\n";
    std::cout << "玩家生命: " << snapshot.player.hp << "/" << snapshot.player.maxHp
              << "  格挡: " << snapshot.player.block
              << "  能量: " << snapshot.energy << "/" << snapshot.energyMax << "\n";
    std::cout << snapshot.enemyName << "生命: " << snapshot.enemy.hp << "/" << snapshot.enemy.maxHp
              << "  格挡: " << snapshot.enemy.block << "\n";

    if (snapshot.enemyIntent.known) {
        std::cout << "敌人意图: " << describeEnemyIntent(snapshot.enemyIntent) << "\n";
    }
}

void printHand(const BridgeBattleSnapshot& snapshot) {
    std::cout << "手牌:\n";
    for (int i = 0; i < static_cast<int>(snapshot.hand.size()); ++i) {
        const BridgeCardState& card = snapshot.hand[i];
        if (card.name.empty()) {
            std::cout << "  [" << i << "] <无效卡牌>\n";
            continue;
        }

        std::cout << "  [" << i << "] " << card.name << "  费用=" << card.cost
                  << "  效果=" << describeEffects(card.effects) << "\n";
    }

    if (snapshot.hand.empty()) {
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
    case PlayResult::InvalidPhase:
        std::cout << "当前阶段不能执行该操作。\n";
        break;
    }
}

}  // namespace

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 演示程序通过 BattleBridge 间接驱动战斗核心，这和未来的 Godot 接法一致。
    BattleBridge bridge;
    bridge.setRandomSeed(static_cast<unsigned int>(std::time(nullptr)));
    bridge.startDefaultBattle();
    bridge.advanceUntilNextDecision();
    bridge.consumeEvents();

    std::cout << "简单卡牌战斗演示\n";
    printHelp();

    int lastShownTurn = -1;
    while (!bridge.isBattleOver()) {
        const BridgeBattleSnapshot snapshot = bridge.getSnapshot();
        if (snapshot.phase == "player_input" && snapshot.turn != lastShownTurn) {
            std::cout << "\n玩家回合开始。\n";
            lastShownTurn = snapshot.turn;
        }

        if (snapshot.phase == "player_input") {
            printStatus(snapshot);
            printHand(snapshot);
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

                if (handIndex >= 0 && handIndex < static_cast<int>(snapshot.hand.size())) {
                    const BridgeCardState card = snapshot.hand[handIndex];
                    const BridgeBattleSnapshot before = snapshot;
                    const PlayResult result = bridge.playCardByInstanceId(card.instanceId);
                    // 演示程序选择“直接推进到下一个决策点”，省略中间逐步动画。
                    bridge.advanceUntilNextDecision();
                    const BridgeBattleSnapshot after = bridge.getSnapshot();
                    bridge.consumeEvents();

                    if (result == PlayResult::Ok) {
                        std::cout << describePlayedCardResult(card, before, after) << "\n";
                    } else {
                        printPlayResult(result);
                    }
                    continue;
                }

                const PlayResult result = bridge.playCardAtHandIndex(handIndex);
                bridge.consumeEvents();
                printPlayResult(result);
            } else if (command == "e") {
                bridge.endTurn();
                bridge.advanceUntilNextDecision();
                const std::vector<BridgeBattleEvent> events = bridge.consumeEvents();
                if (!bridge.isBattleOver()) {
                    printEnemyTurnResult(events);
                }
            } else if (command == "q") {
                std::cout << "已退出。\n";
                return 0;
            } else {
                printHelp();
            }
        } else {
            // Runner 会把演示程序自动推进回玩家输入阶段。
            continue;
        }
    }

    if (bridge.getSnapshot().phase == "victory") {
        std::cout << "\n你赢了。\n";
    } else if (bridge.getSnapshot().phase == "defeat") {
        std::cout << "\n你输了。\n";
    }

    return 0;
}
