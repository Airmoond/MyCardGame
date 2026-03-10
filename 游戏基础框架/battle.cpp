#include<string>
#include<vector>
#include<random>

#include "battle.h"

// battle.cpp 作为 battle.h 的实现文件，包含了 Battle 类的成员函数定义

void Battle::applyDamage(Entity& target, int damage) {//对目标实体造成伤害，考虑格挡
	int damageAfterBlock = std::max(0, damage - target.block);
	target.block = std::max(0, target.block - damage);
	target.hp -= damageAfterBlock;
}

void Battle::gainBlock(Entity& target, int block) {//给目标实体增加格挡
	target.block += block;
}

void Battle::reshuffleIfNeeded() {//如果牌堆空了，重洗弃牌堆
	if (drawpile.empty()) {
		drawpile = discard;
		discard.clear();
		std::shuffle(drawpile.begin(), drawpile.end(), rng);//使用随机数生成器打乱牌堆
	}
}

int Battle::drawCards(int n) {
	int actualDrawn = 0;
	for (int i = 0; i < n; ++i) {
		if (hand.size() >= handLimit) break;//如果手牌已满，停止抽牌
		reshuffleIfNeeded();//如果牌堆空了，重洗弃牌堆

		if (drawpile.empty()) break;//如果牌堆还是空了，就真的停止抽牌了，就是弃牌堆和抽牌堆都空了
		hand.push_back(drawpile.back());//从牌堆顶抽一张牌加入手牌
		drawpile.pop_back();//从牌堆顶移除这张牌
		actualDrawn++;//记录实际抽到的牌数量
	}
	return actualDrawn;
}

void Battle::startBattle() {
	turn = 0;
	energyMax = 2;
	energy = 2;


	// 初始化玩家和敌人状态
	player = { 50, 50, 0 };//玩家初始HP和最大HP为50，初始格挡为0
	enemy = { 40, 40, 0 };//敌人初始HP和最大HP为40，初始格挡为0

	hand.clear();
	drawpile.clear();
	discard.clear();

	// 初始化牌堆
	cardDefs = {
		{ "Strike", 1, { {EffectType::Damage, 6} } },//攻击卡，消耗1能量，造成6点伤害
		{ "Defend", 1, { {EffectType::Block, 5} } },//防御卡，消耗1能量，获得5点格挡
	};

	// 初始化牌堆,每种卡牌10张
	for (int i = 0; i < 10; ++i) {
		drawpile.push_back({ 0, 0 }); // Strike
		drawpile.push_back({ 1, 0 }); // Defend
	}

	std::shuffle(drawpile.begin(), drawpile.end(), rng);//打乱牌堆

	drawCards(openingHandNum);//抽取开局手牌
}

void Battle::startTurn() {
	turn++;
	player.block = 0;
	if (energyMax < energyCap) energyMax++;//每回合增加能量上限，直到达到最大值
	energy = energyMax;//回合开始时能量恢复到当前上限
	drawCards(drawPerTurn);//抽取回合开始的牌
}

PlayResult Battle::playCard(int handIndex) {
	if (handIndex < 0 || handIndex >= hand.size()) return PlayResult::InvalidIndex;//检查手牌索引是否合法

	CardInstance& card = hand[handIndex];
	CardDef& def = cardDefs[card.defId];

	int totalCost = def.baseCost + card.costDelta;//计算卡牌总能量消耗
	if (energy < totalCost) return PlayResult::NotEnoughEnergy;//检查是否有足够的能量

	energy -= totalCost;//扣除能量

	applyCardEffects(card);

	discard.push_back(card);//将使用过的牌放入弃牌堆
	hand.erase(hand.begin() + handIndex);//从手牌中移除这张牌

	return PlayResult::Ok;//出牌成功
}

void Battle::endTurn() {
	for (const auto& card : hand) {//回合结束时将手牌中的牌全部放入弃牌堆
		discard.push_back(card);
	}
	hand.clear();
}

void Battle::enemyAct() {
	// 简单的敌人行为：每回合攻击玩家，造成5点伤害
	applyDamage(player, 5);
}

void Battle::endBattle() {
	// 这里可以添加结算逻辑，比如显示胜利/失败界面，计算奖励等，现在暂时先空着
}

void Battle::applyCardEffects(const CardInstance& card) {//根据卡牌实例应用其效果
	const CardDef& cardDef = cardDefs[card.defId];//根据卡牌实例的defId找到对应的卡牌定义
	for (const Effect& oneEffect : cardDef.effects) {//遍历卡牌定义中的每个效果，应用到目标实体上
		applyEffect(oneEffect);
	}
}

void Battle::applyEffect(const Effect& effect) {//根据效果类型应用单个效果
	switch (effect.type) {//根据效果类型执行不同的操作
	case EffectType::Damage:
		applyDamage(enemy, effect.value);
		break;
	case EffectType::Block:
		gainBlock(player, effect.value);
		break;
	}
}
