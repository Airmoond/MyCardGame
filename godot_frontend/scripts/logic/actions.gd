# 动作系统 —— GameAction 基类 + ActionQueue + 5种具体动作
class_name GameAction
extends RefCounted

# 注意：这里不能用 Battle 类型标注，否则会与 battle.gd 形成循环依赖
func execute(battle) -> void:
	pass


# --- ActionQueue ---
class ActionQueue extends RefCounted:
	var _actions: Array[GameAction] = []

	func push(action: GameAction) -> void:
		_actions.append(action)

	func empty() -> bool:
		return _actions.is_empty()

	func clear() -> void:
		_actions.clear()

	func execute_next(battle) -> void:
		if _actions.is_empty():
			return
		var action: GameAction = _actions.pop_front()
		action.execute(battle)

	func execute_all(battle) -> void:
		while not _actions.is_empty() and not battle.is_battle_over():
			execute_next(battle)
			battle.update_outcome()


# --- PlayCardAction ---
class PlayCardAction extends GameAction:
	var _card: Dictionary = {}  # CardInstance

	func _init(card: Dictionary) -> void:
		_card = card

	func execute(battle) -> void:
		battle.expand_card_effects(_card)


# --- DamageAction ---
class DamageAction extends GameAction:
	var _target: int = BattleEnums.EntityId.ENEMY
	var _amount: int = 0

	func _init(target: int, amount: int) -> void:
		_target = target
		_amount = amount

	func execute(battle) -> void:
		battle.apply_damage(_target, _amount)


# --- GainBlockAction ---
class GainBlockAction extends GameAction:
	var _target: int = BattleEnums.EntityId.PLAYER
	var _amount: int = 0

	func _init(target: int, amount: int) -> void:
		_target = target
		_amount = amount

	func execute(battle) -> void:
		battle.gain_block(_target, _amount)


# --- DrawCardsAction ---
class DrawCardsAction extends GameAction:
	var _amount: int = 0

	func _init(amount: int) -> void:
		_amount = amount

	func execute(battle) -> void:
		battle.draw_cards(_amount)


# --- EnemyAttackAction ---
class EnemyAttackAction extends GameAction:
	var _amount: int = 0

	func _init(amount: int) -> void:
		_amount = amount

	func execute(battle) -> void:
		battle.queue_action(DamageAction.new(BattleEnums.EntityId.PLAYER, _amount))
