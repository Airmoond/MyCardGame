# 玩家信息显示
extends Control

@onready var hp_label: Label = $HPLabel
@onready var hp_bar: ProgressBar = $HPBar
@onready var block_label: Label = $BlockLabel


func update_display(hp: int, max_hp: int, block: int) -> void:
	hp_label.text = "生命: %d / %d" % [hp, max_hp]
	hp_bar.max_value = max_hp
	hp_bar.value = hp

	if block > 0:
		block_label.text = "格挡: %d" % block
		block_label.visible = true
	else:
		block_label.visible = false
