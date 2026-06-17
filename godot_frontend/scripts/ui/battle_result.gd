# 战斗结果面板
extends Control

@onready var result_label: Label = $Panel/ResultLabel
@onready var restart_button: Button = $Panel/RestartButton
@onready var quit_button: Button = $Panel/QuitButton


func _ready() -> void:
	visible = false
	restart_button.pressed.connect(_on_restart)
	quit_button.pressed.connect(_on_quit)


func show_victory() -> void:
	result_label.text = "胜利!"
	result_label.add_theme_color_override("font_color", Color(0.2, 1.0, 0.3))
	_show()


func show_defeat() -> void:
	result_label.text = "失败!"
	result_label.add_theme_color_override("font_color", Color(1.0, 0.2, 0.2))
	_show()


func _show() -> void:
	visible = true
	modulate.a = 0.0
	scale = Vector2(0.8, 0.8)
	var tween := create_tween().set_parallel(true)
	tween.set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	tween.tween_property(self, "modulate:a", 1.0, 0.4)
	tween.tween_property(self, "scale", Vector2(1.0, 1.0), 0.4)


func _on_restart() -> void:
	get_tree().reload_current_scene()


func _on_quit() -> void:
	get_tree().quit()
