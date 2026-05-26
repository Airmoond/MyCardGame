# 手牌容器
extends Control

signal card_clicked(instance_id: int)

const CARD_WIDGET := preload("res://scenes/card_widget.tscn")


func populate_hand(card_views: Array[Dictionary], current_energy: int) -> void:
	# 清除旧卡牌
	for child in get_children():
		child.queue_free()

	# 创建新卡牌
	for i in range(card_views.size()):
		var card_view: Dictionary = card_views[i]
		var widget: Control = CARD_WIDGET.instantiate()
		add_child(widget)
		widget.position.x = i * 170
		var can_afford: bool = card_view.cost <= current_energy
		widget.setup(card_view.card_name, card_view.effects_text, card_view.cost, can_afford, card_view.instance_id)
		widget.card_clicked.connect(_on_widget_clicked)


func _on_widget_clicked(instance_id: int) -> void:
	card_clicked.emit(instance_id)
