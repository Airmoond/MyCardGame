# 战斗主控制器 —— 持有 Battle 实例，连接逻辑层和 UI
extends Control

# --- 场景引用 ---
@onready var player_hud: Control = $PlayerHUD
@onready var enemy_display: Control = $EnemyDisplay
@onready var card_hand: Control = $CardHand
@onready var energy_display: Control = $EnergyDisplay
@onready var pile_display: Control = $PileDisplay
@onready var end_turn_button: Button = $EndTurnButton
@onready var battle_result: Control = $BattleResult
@onready var turn_banner: Label = $TurnBanner

# --- 内部状态 ---
var _battle: Battle = null
var _animating: bool = false


func _ready() -> void:
	_init_battle()
	_start_battle_flow()


func _init_battle() -> void:
	var config := GameContent.create_default_battle_config()
	_battle = Battle.new()
	_battle.initialize(config)

	# 连接信号用于动画
	_battle.damage_applied.connect(_on_damage_applied)
	_battle.block_changed.connect(_on_block_changed)
	_battle.card_moved.connect(_on_card_moved)
	_battle.deck_shuffled.connect(_on_deck_shuffled)

	# 连接 UI 信号
	card_hand.card_clicked.connect(_on_card_clicked)
	end_turn_button.pressed.connect(_on_end_turn)


func _start_battle_flow() -> void:
	# 推进到第一个玩家决策点
	_advance_to_player_input()


func _advance_to_player_input() -> void:
	_animating = true
	end_turn_button.disabled = true

	while _battle.has_pending_actions() or not _battle.can_accept_input():
		if _battle.is_battle_over():
			break
		_battle.advance()
		var events := _battle.consume_events()
		if not events.is_empty():
			await _animate_events(events)

	_refresh_all_ui()

	if _battle.is_battle_over():
		_show_battle_result()
	else:
		end_turn_button.disabled = false
		_animating = false


# --- 事件动画分发 ---
func _animate_events(events: Array[Dictionary]) -> void:
	for event in events:
		match event.type:
			BattleEnums.BattleEventType.TURN_STARTED:
				await _show_turn_banner(event.turn)
			BattleEnums.BattleEventType.DECK_SHUFFLED:
				await _animate_shuffle()
			BattleEnums.BattleEventType.DAMAGE_APPLIED:
				await _animate_damage(event)
			BattleEnums.BattleEventType.BLOCK_CHANGED:
				pass  # 格挡变化在 _refresh_all_ui 中即时反映
			BattleEnums.BattleEventType.CARD_MOVED:
				await _animate_card_move(event)
		# 控制事件回放速度
		await get_tree().create_timer(0.05).timeout


func _show_turn_banner(turn: int) -> void:
	turn_banner.text = "第 %d 回合" % turn
	turn_banner.modulate.a = 1.0
	turn_banner.scale = Vector2(0.5, 0.5)

	var tween := create_tween().set_parallel(true)
	tween.set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	tween.tween_property(turn_banner, "scale", Vector2(1.0, 1.0), 0.3)
	tween.tween_property(turn_banner, "modulate:a", 0.0, 1.2).set_delay(0.8)
	await tween.finished


func _animate_damage(event: Dictionary) -> void:
	if event.amount <= 0 and event.amount2 <= 0:
		return

	var display: Control = player_hud if event.entity == BattleEnums.EntityId.PLAYER else enemy_display

	# 震动动画
	var original_pos := display.position
	var shake_tween := create_tween()
	for _i in range(3):
		shake_tween.tween_property(display, "position:x", original_pos.x + 4, 0.03)
		shake_tween.tween_property(display, "position:x", original_pos.x - 4, 0.03)
	shake_tween.tween_property(display, "position", original_pos, 0.03)

	# 闪红
	var flash_tween := create_tween()
	flash_tween.tween_property(display, "modulate", Color.RED, 0.06)
	flash_tween.tween_property(display, "modulate", Color.WHITE, 0.06)

	# 伤害数字
	if event.amount > 0:
		_spawn_damage_popup(display.global_position, "-%d" % event.amount)

	await shake_tween.finished


func _animate_card_move(event: Dictionary) -> void:
	# 不需要复杂的动画 — hand 会在 refresh 时重建
	await get_tree().create_timer(0.08).timeout


func _animate_shuffle() -> void:
	# 简单的洗牌提示
	pile_display.flash_shuffle()
	await get_tree().create_timer(0.3).timeout


func _spawn_damage_popup(pos: Vector2, text: String) -> void:
	var popup := Label.new()
	popup.text = text
	popup.add_theme_font_size_override("font_size", 28)
	popup.add_theme_color_override("font_color", Color(1.0, 0.2, 0.2))
	popup.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	popup.position = pos + Vector2(0, -20)
	popup.scale = Vector2(0.8, 0.8)
	add_child(popup)

	var tween := create_tween().set_parallel(true)
	tween.tween_property(popup, "position:y", pos.y - 80, 0.8)
	tween.tween_property(popup, "modulate:a", 0.0, 0.8)
	tween.tween_property(popup, "scale", Vector2(1.2, 1.2), 0.15)
	tween.chain().tween_property(popup, "scale", Vector2(1.0, 1.0), 0.15)
	await tween.finished
	popup.queue_free()


# --- UI 刷新 ---
func _refresh_all_ui() -> void:
	var snap := _battle.get_snapshot()

	player_hud.update_display(snap.player.hp, snap.player.max_hp, snap.player.block)
	enemy_display.update_display(snap.enemy.hp, snap.enemy.max_hp, snap.enemy.block, snap.enemy_data.enemy_name)
	enemy_display.update_intent(snap.enemy_intent)
	energy_display.update_energy(snap.energy, snap.energy_max)
	pile_display.update_counts(snap.drawpile_count, snap.discard_count)
	card_hand.populate_hand(snap.hand, snap.energy)


# --- 输入处理 ---
func _on_card_clicked(instance_id: int) -> void:
	if _animating:
		return
	if not _battle.can_accept_input():
		return

	var result := _battle.try_play_card_by_instance_id(instance_id)
	if result == BattleEnums.PlayResult.OK:
		_advance_to_player_input()
	elif result == BattleEnums.PlayResult.NOT_ENOUGH_ENERGY:
		energy_display.flash_error()
		await get_tree().create_timer(0.3).timeout
		energy_display.flash_error_end()


func _on_end_turn() -> void:
	if _animating:
		return

	_battle.end_player_turn()
	_advance_to_player_input()


func _show_battle_result() -> void:
	_animating = true
	end_turn_button.disabled = true

	var snap := _battle.get_snapshot()
	if snap.phase == BattleEnums.BattlePhase.VICTORY:
		battle_result.show_victory()
	else:
		battle_result.show_defeat()

	_animating = false


# --- 信号回调（供需要即时反馈的情况）---
func _on_damage_applied(target: int, amount: int, blocked: int) -> void:
	_refresh_all_ui()


func _on_block_changed(target: int, delta: int, new_block: int) -> void:
	_refresh_all_ui()


func _on_card_moved(instance_id: int, def_id: int, from_zone: int, to_zone: int) -> void:
	_refresh_all_ui()


func _on_deck_shuffled(amount: int) -> void:
	_refresh_all_ui()
