#pragma once
#include<string>
#include<vector>
#include<random>

enum class EffectType//卡牌效果列表
{
	Damage, Block
};

enum class PlayResult//出牌结果
{
	Ok, InvalidIndex, NotEnoughEnergy
};

struct Effect//单个效果结构体
{
	EffectType type;
	int value;//数值，伤害或格挡量
};

struct CardDef//卡牌定义结构体
{
	std::string name;
	int baseCost;//基础能量消耗
	std::vector<Effect> effects;//卡牌效果列表
};

struct CardInstance//卡牌实例结构体（实际卡牌）
{
	int defId;//对应的卡牌定义ID
	int costDelta;//未来可以添加更多属性，如升级状态、特殊标记等
};

struct Entity
{
	int hp;
	int maxHp;
	int block;
};

class Battle
{
public:
	Entity player, enemy;
	int turn = 1;
	int energy = 2;
	int energyCap = 5;//能量上限，最大5
	int energyMax = 2;//当前能量上限
	int openingHandNum = 4;//开局手牌数量
	int drawPerTurn = 2;//每回合抽牌数量
	int handLimit = 10;//手牌上限

	std::vector<CardDef> cardDefs;//卡牌定义列表，这里的意思是，每个CardInstance中的defId对应cardDefs这个vector中的一个CardDef，defId就是索引（0123这样的）
	std::vector<CardInstance> hand, drawpile, discard;//手牌、牌堆、弃牌堆

	std::mt19937 rng;
private:

public:
	void startBattle();
	void startTurn();
	int drawCards(int n);//抽牌，返回实际抽到的牌数量
	PlayResult playCard(int handIndex);//出牌，返回结果

	void applyCardEffects(const CardInstance& card);//应用卡牌效果
	void applyEffect(const Effect& effect, Entity& target);//应用单个效果

	void endTurn();
	void enemyAct();
	void endBattle();

	//以下是具体执行的helper函数
	//调用链：playCard → applyCardEffects(card) → (for each effect) applyEffect(effect, source, target) → helpers(applyDamage/gainBlock/draw...)

	void reshuffleIfNeeded();//如果牌堆空了，重洗弃牌堆

	void applyDamage(Entity& target, int damage);//造成伤害
	void gainBlock(Entity& target, int block);//获得格挡
};

