# 单张卡牌显示
extends Control

signal card_clicked(instance_id: int)

var _instance_id: int = -1

@onready var panel: Panel = $Panel
@onready var name_label: Label = $Panel/NameLabel
@onready var cost_label: Label = $Panel/CostLabel
@onready var effect_label: Label = $Panel/EffectLabel


func _ready() -> void:
	# 让所有子控件忽略鼠标事件，确保根 Control 能收到点击
	mouse_filter = MOUSE_FILTER_STOP
	_ignore_mouse_for_children(self)


func _ignore_mouse_for_children(node: Node) -> void:
	for child in node.get_children():
		if child is Control and child != self:
			child.mouse_filter = MOUSE_FILTER_IGNORE
		_ignore_mouse_for_children(child)


func setup(card_name: String, effect_text: String, card_cost: int, can_afford: bool, instance_id: int) -> void:
	_instance_id = instance_id

	name_label.text = card_name
	effect_label.text = effect_text
	cost_label.text = str(card_cost)

	if can_afford:
		cost_label.add_theme_color_override("font_color", Color.WHITE)
		modulate = Color.WHITE
	else:
		cost_label.add_theme_color_override("font_color", Color(1.0, 0.3, 0.3))
		modulate = Color(0.6, 0.6, 0.6)


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		card_clicked.emit(_instance_id)


