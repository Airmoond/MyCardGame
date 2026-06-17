# 默认战斗配置工厂
class_name GameContent
extends RefCounted

static func create_default_battle_config() -> BattleConfig:
	var config := BattleConfig.new()
	config.player_max_hp = 50
	config.starting_energy = 2
	config.energy_cap = 5
	config.opening_hand_num = 4
	config.draw_per_turn = 2
	config.hand_limit = 10

	# 加载敌人
	var enemy := EnemyData.new()
	enemy.enemy_name = "训练木桩"
	enemy.max_hp = 40
	enemy.attack_damage = 6
	config.enemy = enemy

	# 加载卡牌定义
	var strike_effect := EffectData.new()
	strike_effect.effect_type = BattleEnums.EffectType.DAMAGE
	strike_effect.value = 6
	strike_effect.target = BattleEnums.EntityId.ENEMY

	var strike := CardData.new()
	strike.id = "strike"
	strike.card_name = "打击"
	strike.base_cost = 1
	strike.effects.append(strike_effect)

	var defend_effect := EffectData.new()
	defend_effect.effect_type = BattleEnums.EffectType.BLOCK
	defend_effect.value = 5
	defend_effect.target = BattleEnums.EntityId.PLAYER

	var defend := CardData.new()
	defend.id = "defend"
	defend.card_name = "防御"
	defend.base_cost = 1
	defend.effects.append(defend_effect)

	config.card_defs = [strike, defend]

	# 10 张打击 + 10 张防御
	for _i in range(10):
		config.starting_deck.append(strike)
		config.starting_deck.append(defend)

	return config
