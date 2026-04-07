# Godot 接入说明

## 推荐入口

如果你准备把当前战斗逻辑接入 Godot，推荐把 `BattleBridge` 作为表现层与逻辑层之间的边界。

相关文件：

- `游戏基础框架/battle_bridge.h`
- `游戏基础框架/battle_bridge.cpp`
- `游戏基础框架/battle.h`
- `游戏基础框架/battle.cpp`
- `游戏基础框架/battle_types.h`

## 为什么优先用 BattleBridge

`Battle` 是核心规则类，负责真正的战斗推进和状态变化。

`BattleBridge` 是面向表现层的包装层，主要做两件事：

- 把底层枚举转换成更容易在前端使用的字符串 token
- 把运行时卡牌、快照和事件整理成更适合 UI 读取的结构

如果直接把 Godot 连到 `Battle`，也不是不行，但你会在表现层里写更多转换代码。

## 推荐调用流程

### 开局

1. 创建一个 `BattleBridge`
2. 调用 `setRandomSeed()`，如果你希望可复现调试
3. 调用 `startDefaultBattle()` 或 `startBattle()`
4. 立刻调用一次 `advanceUntilNextDecision()`
5. 调用 `getSnapshot()` 构建初始 UI
6. 调用 `consumeEvents()` 播放开局和回合开始事件

### 玩家出牌

1. 从 `getSnapshot()` 里读取手牌
2. 拿到玩家点击的那张牌的 `instanceId`
3. 调用 `playCardByInstanceId(instanceId)`
4. 如果你想省略中间动画，直接调用 `advanceUntilNextDecision()`
5. 如果你想逐步播放动画，就循环调用 `advanceOneStep()`
6. 每推进一次后，调用 `consumeEvents()`
7. 播完事件后，再用 `getSnapshot()` 刷新 UI

### 结束回合

1. 调用 `endTurn()`
2. 根据你的表现方式选择：
   - 同步推进：`advanceUntilNextDecision()`
   - 分步推进：循环 `advanceOneStep()`
3. 每一步都读取 `consumeEvents()`
4. 最后用 `getSnapshot()` 刷新界面

## BattleBridge 关键接口

- `startDefaultBattle()`
- `startBattle()`
- `playCardAtHandIndex(int handIndex)`
- `playCardByInstanceId(int instanceId)`
- `endTurn()`
- `advanceOneStep()`
- `advanceUntilNextDecision()`
- `getSnapshot()`
- `consumeEvents()`
- `getCardCatalog()`

## 哪些接口更适合 Godot

更推荐：

- `playCardByInstanceId()`
- `getSnapshot()`
- `consumeEvents()`
- `advanceOneStep()`

原因是：

- `instanceId` 能稳定对应某一张具体卡牌
- `snapshot` 适合刷新 UI
- `events` 适合驱动动画
- `advanceOneStep()` 更适合做异步表现

## Godot 侧最小包装思路

如果你准备使用 GDExtension，可以再包一层 Godot 对象，例如 `BattleBridgeObject`。

这个 Godot 对象内部持有一个 `BattleBridge`，然后把下面这些方法暴露给脚本：

- `start_battle()`
- `play_card(instance_id)`
- `end_turn()`
- `advance_one_step()`
- `advance_to_next_decision()`
- `get_snapshot()`
- `consume_events()`

在这层包装里，把 C++ 结构转换成 Godot 的：

- `Dictionary`
- `Array`
- 或者你自己定义的 Godot Resource / Object

## 一条很实用的建议

如果你现在的目标是尽快做出 Godot demo，建议先这样做：

- 逻辑层继续保持现在的结构
- 先写一个最小 GDExtension 包装
- 先让 Godot 能显示血量、能量、手牌和敌人意图
- 再逐步把事件流接成动画

这样推进速度通常比“先做一整套复杂工程拆分”更快，也更容易边做边验证方向。
