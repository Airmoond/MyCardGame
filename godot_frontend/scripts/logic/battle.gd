# 战斗核心状态机 —— 回合推进、卡牌、能量、格挡、牌库管理
class_name Battle
extends RefCounted

# Godot 信号 —— UI 层可以直接连接这些信号驱动动画
signal battle_initialized()
signal phase_changed(new_phase: int)
signal turn_started(turn: int)
signal energy_changed(energy: int, max_energy: int)
signal card_moved(instance_id: int, def_id: int, from_zone: int, to_zone: int)
signal deck_shuffled(amount: int)
signal damage_applied(target: int, amount: int, blocked: int)
signal block_changed(target: int, delta: int, new_block: int)
signal battle_ended(victory: bool)

# --- 内部状态 ---
var _phase: int = BattleEnums.BattlePhase.BATTLE_START
var _player: Dictionary = {}       # { hp, max_hp, block }
var _enemy: Dictionary = {}        # { hp, max_hp, block }
var _turn: int = 0
var _energy: int = 0
var _energy_cap: int = 0
var _energy_max: int = 0
var _opening_hand_num: int = 0
var _draw_per_turn: int = 0
var _hand_limit: int = 0
var _next_card_instance_id: int = 1
var _enemy_data: EnemyData = null
var _card_defs: Array[CardData] = []
var _hand: Array[Dictionary] = []
var _drawpile: Array[Dictionary] = []
var _discard: Array[Dictionary] = []
var _pending_events: Array[Dictionary] = []
var _action_queue: GameAction.ActionQueue = null
var _rng: RandomNumberGenerator = null

# --- 帮助函数：创建常用结构 ---
static func make_entity(hp: int, max_hp: int, block: int = 0) -> Dictionary:
	return {"hp": hp, "max_hp": max_hp, "block": block}

static func make_card_instance(def_id: int, instance_id: int = -1, cost_delta: int = 0) -> Dictionary:
	return {"instance_id": instance_id, "def_id": def_id, "cost_delta": cost_delta}

static func make_card_view(card: Dictionary, cost: int, card_name: String, effects_text: String) -> Dictionary:
	return {"instance_id": card.instance_id, "def_id": card.def_id, "cost": cost, "card_name": card_name, "effects_text": effects_text}

static func make_event(type: int, extra := {}) -> Dictionary:
	var e := {
		"type": type,
		"phase": BattleEnums.BattlePhase.BATTLE_START,
		"entity": BattleEnums.EntityId.PLAYER,
		"amount": 0,
		"amount2": 0,
		"turn": 0,
		"card_instance_id": -1,
		"card_def_id": -1,
		"from_zone": BattleEnums.CardZone.NONE,
		"to_zone": BattleEnums.CardZone.NONE,
	}
	e.merge(extra)
	return e

# --- 初始化 ---
func initialize(config: BattleConfig) -> void:
	_pending_events.clear()
	if _action_queue:
		_action_queue.clear()
	else:
		_action_queue = GameAction.ActionQueue.new()
	_next_card_instance_id = 1
	_rng = RandomNumberGenerator.new()
	_rng.randomize()

	_energy_cap = config.energy_cap
	_energy_max = config.starting_energy
	_energy = _energy_max
	_opening_hand_num = config.opening_hand_num
	_draw_per_turn = config.draw_per_turn
	_hand_limit = config.hand_limit
	_enemy_data = config.enemy
	_card_defs = config.card_defs

	_player = make_entity(config.player_max_hp, config.player_max_hp)
	_enemy = make_entity(_enemy_data.max_hp, _enemy_data.max_hp)
	_turn = 0

	_hand.clear()
	_discard.clear()
	_drawpile.clear()
	for card_data in config.starting_deck:
		var inst := make_card_instance(_get_def_id(card_data))
		_drawpile.append(inst)
	_assign_instance_ids()

	_set_phase(BattleEnums.BattlePhase.BATTLE_START)
	_shuffle_drawpile()

	_record_event(make_event(BattleEnums.BattleEventType.BATTLE_INITIALIZED))
	_record_energy_changed()
	battle_initialized.emit()

	draw_cards(_opening_hand_num)
	update_outcome()


func set_random_seed(seed: int) -> void:
	_rng.seed = seed


# --- 推进 ---
func advance() -> bool:
	if is_battle_over():
		return false

	if _phase == BattleEnums.BattlePhase.BATTLE_START or _phase == BattleEnums.BattlePhase.PLAYER_TURN_START:
		_start_player_turn()
		return true

	if _phase == BattleEnums.BattlePhase.ENEMY_TURN_START:
		_start_enemy_turn()
		return true

	if not _action_queue.empty():
		_action_queue.execute_next(self)
		update_outcome()
		if not is_battle_over() and _action_queue.empty():
			_on_action_queue_drained()
		return true

	return false


# --- 出牌 ---
func try_play_card(hand_index: int) -> int:
	if not can_accept_input():
		return BattleEnums.PlayResult.INVALID_PHASE

	if hand_index < 0 or hand_index >= _hand.size():
		return BattleEnums.PlayResult.INVALID_INDEX

	return try_play_card_by_instance_id(_hand[hand_index].instance_id)


func try_play_card_by_instance_id(instance_id: int) -> int:
	if not can_accept_input():
		return BattleEnums.PlayResult.INVALID_PHASE

	var hand_index := _find_hand_index(instance_id)
	if hand_index < 0:
		return BattleEnums.PlayResult.INVALID_INDEX

	var card: Dictionary = _hand[hand_index]
	if card.def_id < 0 or card.def_id >= _card_defs.size():
		return BattleEnums.PlayResult.INVALID_INDEX

	var total_cost := _compute_card_cost(card)
	if _energy < total_cost:
		return BattleEnums.PlayResult.NOT_ENOUGH_ENERGY

	_energy -= total_cost
	_record_energy_changed()

	_discard.append(card)
	_hand.remove_at(hand_index)

	_record_event(make_event(BattleEnums.BattleEventType.CARD_MOVED, {
		"card_instance_id": card.instance_id,
		"card_def_id": card.def_id,
		"from_zone": BattleEnums.CardZone.HAND,
		"to_zone": BattleEnums.CardZone.DISCARD,
	}))
	card_moved.emit(card.instance_id, card.def_id, BattleEnums.CardZone.HAND, BattleEnums.CardZone.DISCARD)

	queue_action(GameAction.PlayCardAction.new(card))
	_set_phase(BattleEnums.BattlePhase.PLAYER_ACTING)
	update_outcome()

	return BattleEnums.PlayResult.OK


# --- 结束回合 ---
func end_player_turn() -> void:
	if not can_accept_input():
		return

	discard_hand()
	_set_phase(BattleEnums.BattlePhase.ENEMY_TURN_START)
	update_outcome()


func resolve_actions() -> void:
	while not _action_queue.empty() and not is_battle_over():
		advance()


# --- 状态查询 ---
func can_accept_input() -> bool:
	return _phase == BattleEnums.BattlePhase.PLAYER_INPUT and _action_queue.empty() and not is_battle_over()


func is_battle_over() -> bool:
	return _phase == BattleEnums.BattlePhase.VICTORY or _phase == BattleEnums.BattlePhase.DEFEAT


func has_pending_actions() -> bool:
	return not _action_queue.empty()


func get_turn() -> int:
	return _turn


func get_phase() -> int:
	return _phase


func get_snapshot() -> Dictionary:
	var hand_views: Array[Dictionary] = []
	for card in _hand:
		var cdef: CardData = _card_defs[card.def_id] if card.def_id >= 0 and card.def_id < _card_defs.size() else null
		var cname := cdef.card_name if cdef else "???"
		var ctext := cdef.describe_effects() if cdef else ""
		hand_views.append(make_card_view(card, _compute_card_cost(card), cname, ctext))

	return {
		"phase": _phase,
		"turn": _turn,
		"energy": _energy,
		"energy_cap": _energy_cap,
		"energy_max": _energy_max,
		"player": _player.duplicate(),
		"enemy": _enemy.duplicate(),
		"enemy_data": _enemy_data,
		"enemy_intent": get_enemy_intent(),
		"hand": hand_views,
		"drawpile_count": _drawpile.size(),
		"discard_count": _discard.size(),
		"has_pending_actions": not _action_queue.empty(),
	}


func get_enemy_intent() -> Dictionary:
	if is_battle_over():
		return {"known": false, "type": BattleEnums.EffectType.DAMAGE, "target": BattleEnums.EntityId.PLAYER, "value": 0}

	return {
		"known": true,
		"type": BattleEnums.EffectType.DAMAGE,
		"target": BattleEnums.EntityId.PLAYER,
		"value": _enemy_data.attack_damage,
	}


func consume_events() -> Array[Dictionary]:
	var events := _pending_events.duplicate()
	_pending_events.clear()
	return events


# --- 牌库操作 ---
func draw_cards(n: int) -> int:
	var actual_drawn := 0
	for i in range(n):
		if _hand.size() >= _hand_limit:
			break

		_reshuffle_if_needed()
		if _drawpile.is_empty():
			break

		var card: Dictionary = _drawpile.pop_back()
		_hand.append(card)
		actual_drawn += 1

		_record_event(make_event(BattleEnums.BattleEventType.CARD_MOVED, {
			"card_instance_id": card.instance_id,
			"card_def_id": card.def_id,
			"from_zone": BattleEnums.CardZone.DRAW_PILE,
			"to_zone": BattleEnums.CardZone.HAND,
		}))
		card_moved.emit(card.instance_id, card.def_id, BattleEnums.CardZone.DRAW_PILE, BattleEnums.CardZone.HAND)

	return actual_drawn


func discard_hand() -> void:
	for card in _hand:
		_discard.append(card)
		_record_event(make_event(BattleEnums.BattleEventType.CARD_MOVED, {
			"card_instance_id": card.instance_id,
			"card_def_id": card.def_id,
			"from_zone": BattleEnums.CardZone.HAND,
			"to_zone": BattleEnums.CardZone.DISCARD,
		}))
		card_moved.emit(card.instance_id, card.def_id, BattleEnums.CardZone.HAND, BattleEnums.CardZone.DISCARD)
	_hand.clear()


# --- 动作接口（供 Action 调用）---
func queue_action(action: GameAction) -> void:
	_action_queue.push(action)


func expand_card_effects(card: Dictionary) -> void:
	if card.def_id < 0 or card.def_id >= _card_defs.size():
		return

	var card_def: CardData = _card_defs[card.def_id]
	for effect in card_def.effects:
		_enqueue_effect(effect)


func update_outcome() -> void:
	if _enemy.hp <= 0:
		_enemy.hp = 0
		if _phase != BattleEnums.BattlePhase.VICTORY:
			_set_phase(BattleEnums.BattlePhase.VICTORY)
			_record_event(make_event(BattleEnums.BattleEventType.BATTLE_ENDED, {
				"entity": BattleEnums.EntityId.ENEMY,
			}))
			battle_ended.emit(true)
	elif _player.hp <= 0:
		_player.hp = 0
		if _phase != BattleEnums.BattlePhase.DEFEAT:
			_set_phase(BattleEnums.BattlePhase.DEFEAT)
			_record_event(make_event(BattleEnums.BattleEventType.BATTLE_ENDED, {
				"entity": BattleEnums.EntityId.PLAYER,
			}))
			battle_ended.emit(false)


func apply_damage(target_id: int, damage: int) -> void:
	var target: Dictionary = _player if target_id == BattleEnums.EntityId.PLAYER else _enemy
	var blocked: int = mini(target.block, max(0, damage))
	var applied: int = max(0, damage - blocked)

	if blocked != 0:
		target.block -= blocked
		_record_block_changed(target_id, -blocked, target.block)
		block_changed.emit(target_id, -blocked, target.block)

	if applied != 0:
		target.hp = max(0, target.hp - applied)

	_record_event(make_event(BattleEnums.BattleEventType.DAMAGE_APPLIED, {
		"entity": target_id,
		"amount": applied,
		"amount2": blocked,
	}))
	damage_applied.emit(target_id, applied, blocked)


func gain_block(target_id: int, block: int) -> void:
	if block == 0:
		return

	var target := _player if target_id == BattleEnums.EntityId.PLAYER else _enemy
	target.block += block
	_record_block_changed(target_id, block, target.block)
	block_changed.emit(target_id, block, target.block)


# --- 内部函数 ---
func _set_phase(phase: int) -> void:
	if _phase == phase:
		return
	_phase = phase
	_record_event(make_event(BattleEnums.BattleEventType.PHASE_CHANGED, {"phase": phase}))
	phase_changed.emit(phase)


func _record_event(event: Dictionary) -> void:
	event.phase = _phase
	event.turn = _turn
	_pending_events.append(event)


func _record_energy_changed() -> void:
	_record_event(make_event(BattleEnums.BattleEventType.ENERGY_CHANGED, {
		"amount": _energy,
		"amount2": _energy_max,
	}))
	energy_changed.emit(_energy, _energy_max)


func _record_block_changed(target_id: int, delta: int, new_block: int) -> void:
	_record_event(make_event(BattleEnums.BattleEventType.BLOCK_CHANGED, {
		"entity": target_id,
		"amount": delta,
		"amount2": new_block,
	}))


func _start_player_turn() -> void:
	if is_battle_over():
		return

	_set_phase(BattleEnums.BattlePhase.PLAYER_TURN_START)
	_turn += 1

	_record_event(make_event(BattleEnums.BattleEventType.TURN_STARTED))
	turn_started.emit(_turn)

	# 清除玩家格挡
	if _player.block != 0:
		var lost_block: int = -_player.block
		_player.block = 0
		_record_block_changed(BattleEnums.EntityId.PLAYER, lost_block, _player.block)
		block_changed.emit(BattleEnums.EntityId.PLAYER, lost_block, _player.block)

	# 能量上限递增（第一回合不增）
	if _energy_max < _energy_cap and _turn > 1:
		_energy_max += 1

	_energy = _energy_max
	_record_energy_changed()

	# 第一回合已抽过开局手牌，第二回合起每回合抽牌
	if _turn > 1:
		draw_cards(_draw_per_turn)

	_set_phase(BattleEnums.BattlePhase.PLAYER_INPUT)
	update_outcome()


func _start_enemy_turn() -> void:
	if is_battle_over():
		return

	_set_phase(BattleEnums.BattlePhase.ENEMY_TURN_START)

	# 清除敌人格挡
	if _enemy.block != 0:
		var lost_block: int = -_enemy.block
		_enemy.block = 0
		_record_block_changed(BattleEnums.EntityId.ENEMY, lost_block, _enemy.block)
		block_changed.emit(BattleEnums.EntityId.ENEMY, lost_block, _enemy.block)

	queue_action(GameAction.EnemyAttackAction.new(_enemy_data.attack_damage))
	_set_phase(BattleEnums.BattlePhase.ENEMY_ACTING)


func _on_action_queue_drained() -> void:
	if _phase == BattleEnums.BattlePhase.PLAYER_ACTING:
		_set_phase(BattleEnums.BattlePhase.PLAYER_INPUT)
	elif _phase == BattleEnums.BattlePhase.ENEMY_ACTING:
		_set_phase(BattleEnums.BattlePhase.PLAYER_TURN_START)


func _compute_card_cost(card: Dictionary) -> int:
	if card.def_id < 0 or card.def_id >= _card_defs.size():
		return 0
	return max(0, _card_defs[card.def_id].base_cost + card.cost_delta)


func _enqueue_effect(effect: EffectData) -> void:
	match effect.effect_type:
		BattleEnums.EffectType.DAMAGE:
			queue_action(GameAction.DamageAction.new(effect.target, effect.value))
		BattleEnums.EffectType.BLOCK:
			queue_action(GameAction.GainBlockAction.new(effect.target, effect.value))
		BattleEnums.EffectType.DRAW:
			queue_action(GameAction.DrawCardsAction.new(effect.value))


func _find_hand_index(instance_id: int) -> int:
	for i in range(_hand.size()):
		if _hand[i].instance_id == instance_id:
			return i
	return -1


func _get_def_id(card_data: CardData) -> int:
	for i in range(_card_defs.size()):
		if _card_defs[i] == card_data:
			return i
	return -1


func _assign_instance_ids() -> void:
	for i in range(_drawpile.size()):
		_drawpile[i].instance_id = _next_card_instance_id
		_next_card_instance_id += 1


func _shuffle_drawpile() -> void:
	# Fisher-Yates 洗牌，使用本地 RNG 保证可复现
	for i in range(_drawpile.size() - 1, 0, -1):
		var j := _rng.randi_range(0, i)
		var tmp = _drawpile[i]
		_drawpile[i] = _drawpile[j]
		_drawpile[j] = tmp


func _reshuffle_if_needed() -> void:
	if not _drawpile.is_empty() or _discard.is_empty():
		return

	_drawpile = _discard.duplicate()
	_discard.clear()
	_shuffle_drawpile()

	_record_event(make_event(BattleEnums.BattleEventType.DECK_SHUFFLED, {
		"amount": _drawpile.size(),
	}))
	deck_shuffled.emit(_drawpile.size())
