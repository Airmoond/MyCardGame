# 牌堆显示
extends Control

@onready var drawpile_label: Label = $DrawPile
@onready var discard_label: Label = $Discard


func update_counts(drawpile: int, discard: int) -> void:
	drawpile_label.text = "抽牌堆: %d" % drawpile
	discard_label.text = "弃牌堆: %d" % discard


func flash_shuffle() -> void:
	drawpile_label.add_theme_color_override("font_color", Color(0.4, 0.7, 1.0))
	var tween := create_tween()
	tween.tween_property(drawpile_label, "modulate", Color.WHITE, 0.4)
	drawpile_label.add_theme_color_override("font_color", Color.WHITE)
