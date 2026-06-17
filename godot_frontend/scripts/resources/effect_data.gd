# 单条卡牌效果定义
class_name EffectData
extends Resource

@export_enum("Damage:0", "Block:1", "Draw:2") var effect_type: int = 0
@export var value: int = 0
@export_enum("Player:0", "Enemy:1") var target: int = 1
