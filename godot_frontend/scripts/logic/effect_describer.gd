# 效果中文描述工具
class_name EffectDescriber
extends RefCounted

static func describe_target(target: int) -> String:
	if target == BattleEnums.EntityId.PLAYER:
		return "自己"
	elif target == BattleEnums.EntityId.ENEMY:
		return "敌人"
	return "未知目标"

static func describe(effect: EffectData) -> String:
	if effect.effect_type == BattleEnums.EffectType.DAMAGE:
		return "对" + describe_target(effect.target) + "造成" + str(effect.value) + "点伤害"
	elif effect.effect_type == BattleEnums.EffectType.BLOCK:
		return describe_target(effect.target) + "获得" + str(effect.value) + "点格挡"
	elif effect.effect_type == BattleEnums.EffectType.DRAW:
		return "抽" + str(effect.value) + "张牌"
	return "未知效果"
