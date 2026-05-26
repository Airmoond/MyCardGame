# 卡牌原型定义
class_name CardData
extends Resource

@export var id: String = ""
@export var card_name: String = ""
@export var base_cost: int = 0
@export var effects: Array[EffectData] = []

func describe_effects() -> String:
	if effects.is_empty():
		return "无效果"
	var parts: Array[String] = []
	for e in effects:
		parts.append(EffectDescriber.describe(e))
	return "，".join(parts)
