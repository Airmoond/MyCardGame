# 能量显示
extends Control

@onready var label: Label = $Label
var _error_flash_tween: Tween = null


func update_energy(current: int, max_energy: int) -> void:
	label.text = "能量: %d / %d" % [current, max_energy]


func flash_error() -> void:
	label.add_theme_color_override("font_color", Color(1.0, 0.2, 0.2))


func flash_error_end() -> void:
	label.add_theme_color_override("font_color", Color.WHITE)
