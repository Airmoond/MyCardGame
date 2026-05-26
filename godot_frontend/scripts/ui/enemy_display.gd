# 敌人信息显示
extends Control

@onready var name_label: Label = $NameLabel
@onready var hp_label: Label = $HPLabel
@onready var hp_bar: ProgressBar = $HPBar
@onready var block_label: Label = $BlockLabel
@onready var intent_label: Label = $IntentLabel


func update_display(hp: int, max_hp: int, block: int, enemy_name: String) -> void:
	name_label.text = enemy_name
	hp_label.text = "生命: %d / %d" % [hp, max_hp]
	hp_bar.max_value = max_hp
	hp_bar.value = hp

	if block > 0:
		block_label.text = "格挡: %d" % block
		block_label.visible = true
	else:
		block_label.visible = false


func update_intent(intent: Dictionary) -> void:
	if not intent.known:
		intent_label.text = "意图: ???"
		return

	if intent.type == BattleEnums.EffectType.DAMAGE and intent.target == BattleEnums.EntityId.PLAYER:
		intent_label.text = "意图: 造成 %d 点伤害" % intent.value
	elif intent.type == BattleEnums.EffectType.BLOCK:
		intent_label.text = "意图: 获得 %d 点格挡" % intent.value
	else:
		intent_label.text = "意图: 未知"
