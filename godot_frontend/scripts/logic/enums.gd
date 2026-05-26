# 战斗系统所有枚举定义
class_name BattleEnums
extends RefCounted

enum EffectType {
	DAMAGE = 0,
	BLOCK = 1,
	DRAW = 2,
}

enum EntityId {
	PLAYER = 0,
	ENEMY = 1,
}

enum CardZone {
	NONE = 0,
	DRAW_PILE = 1,
	HAND = 2,
	DISCARD = 3,
}

enum BattlePhase {
	BATTLE_START = 0,
	PLAYER_TURN_START = 1,
	PLAYER_INPUT = 2,
	PLAYER_ACTING = 3,
	ENEMY_TURN_START = 4,
	ENEMY_ACTING = 5,
	VICTORY = 6,
	DEFEAT = 7,
}

enum PlayResult {
	OK = 0,
	INVALID_INDEX = 1,
	NOT_ENOUGH_ENERGY = 2,
	INVALID_PHASE = 3,
}

enum BattleEventType {
	BATTLE_INITIALIZED = 0,
	PHASE_CHANGED = 1,
	TURN_STARTED = 2,
	ENERGY_CHANGED = 3,
	CARD_MOVED = 4,
	DECK_SHUFFLED = 5,
	DAMAGE_APPLIED = 6,
	BLOCK_CHANGED = 7,
	BATTLE_ENDED = 8,
}

# 将 EffectType 转为字符串 token
static func effect_type_name(t: EffectType) -> String:
	match t:
		EffectType.DAMAGE: return "damage"
		EffectType.BLOCK:  return "block"
		EffectType.DRAW:   return "draw"
	return "unknown"

# 将 EntityId 转为字符串 token
static func entity_name(e: EntityId) -> String:
	match e:
		EntityId.PLAYER: return "player"
		EntityId.ENEMY:  return "enemy"
	return "unknown"

# 将 CardZone 转为字符串 token
static func zone_name(z: CardZone) -> String:
	match z:
		CardZone.NONE:       return "none"
		CardZone.DRAW_PILE:  return "draw_pile"
		CardZone.HAND:       return "hand"
		CardZone.DISCARD:    return "discard"
	return "unknown"

# 将 BattlePhase 转为字符串 token
static func phase_name(p: BattlePhase) -> String:
	match p:
		BattlePhase.BATTLE_START:       return "battle_start"
		BattlePhase.PLAYER_TURN_START:  return "player_turn_start"
		BattlePhase.PLAYER_INPUT:       return "player_input"
		BattlePhase.PLAYER_ACTING:      return "player_acting"
		BattlePhase.ENEMY_TURN_START:   return "enemy_turn_start"
		BattlePhase.ENEMY_ACTING:       return "enemy_acting"
		BattlePhase.VICTORY:            return "victory"
		BattlePhase.DEFEAT:             return "defeat"
	return "unknown"

# 将 BattleEventType 转为字符串 token
static func event_type_name(t: BattleEventType) -> String:
	match t:
		BattleEventType.BATTLE_INITIALIZED: return "battle_initialized"
		BattleEventType.PHASE_CHANGED:      return "phase_changed"
		BattleEventType.TURN_STARTED:       return "turn_started"
		BattleEventType.ENERGY_CHANGED:     return "energy_changed"
		BattleEventType.CARD_MOVED:         return "card_moved"
		BattleEventType.DECK_SHUFFLED:      return "deck_shuffled"
		BattleEventType.DAMAGE_APPLIED:     return "damage_applied"
		BattleEventType.BLOCK_CHANGED:      return "block_changed"
		BattleEventType.BATTLE_ENDED:       return "battle_ended"
	return "unknown"
