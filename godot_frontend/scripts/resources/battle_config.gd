# 战斗配置 —— 打包规则参数、敌人和卡组
class_name BattleConfig
extends Resource

@export var player_max_hp: int = 50
@export var starting_energy: int = 2
@export var energy_cap: int = 5
@export var opening_hand_num: int = 4
@export var draw_per_turn: int = 2
@export var hand_limit: int = 10
@export var enemy: EnemyData
@export var card_defs: Array[CardData] = []
# starting_deck 存储的是 CardData 引用列表，Battle 初始化时据此创建 CardInstance
@export var starting_deck: Array[CardData] = []
