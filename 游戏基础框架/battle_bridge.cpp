#include "battle_bridge.h"

#include <utility>

BattleBridge::BattleBridge()
    : config_(createDefaultBattleConfig()) {
}

BattleBridge::BattleBridge(BattleConfig config)
    : config_(std::move(config)) {
}

void BattleBridge::setBattleConfig(BattleConfig config) {
    config_ = std::move(config);
}

void BattleBridge::setRandomSeed(unsigned int seed) {
    battle_.setRandomSeed(seed);
}

void BattleBridge::startDefaultBattle() {
    // 便于调试和演示：直接用默认内容配置开一局。
    config_ = createDefaultBattleConfig();
    startBattle();
}

void BattleBridge::startBattle() {
    // Bridge 本身不自动推进到玩家输入阶段，把“推进节奏”的控制权留给调用方。
    battle_.initialize(config_);
}

PlayResult BattleBridge::playCardAtHandIndex(int handIndex) {
    return battle_.tryPlayCard(handIndex);
}

PlayResult BattleBridge::playCardByInstanceId(int instanceId) {
    return battle_.tryPlayCardByInstanceId(instanceId);
}

bool BattleBridge::endTurn() {
    if (!battle_.canAcceptInput()) {
        return false;
    }

    battle_.endPlayerTurn();
    return true;
}

bool BattleBridge::advanceOneStep() {
    return battle_.advance();
}

bool BattleBridge::advanceUntilNextDecision() {
    // 这个接口适合“动画省略”的同步前端。
    // 对 Godot 来说，如果想逐步播演出，可以改用 advanceOneStep。
    bool advanced = false;
    while (!battle_.isBattleOver() && !battle_.canAcceptInput()) {
        if (!battle_.advance()) {
            break;
        }
        advanced = true;
    }

    return advanced;
}

bool BattleBridge::canAcceptInput() const {
    return battle_.canAcceptInput();
}

bool BattleBridge::isBattleOver() const {
    return battle_.isBattleOver();
}

BridgeBattleSnapshot BattleBridge::getSnapshot() const {
    const BattleSnapshot snapshot = battle_.getSnapshot();

    // 这里把核心层枚举转换成字符串 token，降低表现层绑定成本。
    BridgeBattleSnapshot bridgeSnapshot;
    bridgeSnapshot.phase = toToken(snapshot.phase);
    bridgeSnapshot.turn = snapshot.turn;
    bridgeSnapshot.energy = snapshot.energy;
    bridgeSnapshot.energyCap = snapshot.energyCap;
    bridgeSnapshot.energyMax = snapshot.energyMax;
    bridgeSnapshot.enemyName = snapshot.enemyData.name;
    bridgeSnapshot.player = { snapshot.player.hp, snapshot.player.maxHp, snapshot.player.block };
    bridgeSnapshot.enemy = { snapshot.enemy.hp, snapshot.enemy.maxHp, snapshot.enemy.block };
    bridgeSnapshot.enemyIntent = {
        snapshot.enemyIntent.known,
        toToken(snapshot.enemyIntent.type),
        toToken(snapshot.enemyIntent.target),
        snapshot.enemyIntent.value,
    };
    bridgeSnapshot.drawpileCount = snapshot.drawpileCount;
    bridgeSnapshot.discardCount = snapshot.discardCount;
    bridgeSnapshot.hasPendingActions = snapshot.hasPendingActions;
    bridgeSnapshot.canAcceptInput = battle_.canAcceptInput();
    bridgeSnapshot.isBattleOver = battle_.isBattleOver();

    bridgeSnapshot.hand.reserve(snapshot.hand.size());
    for (const CardView& card : snapshot.hand) {
        bridgeSnapshot.hand.push_back(makeCardState(card));
    }

    return bridgeSnapshot;
}

std::vector<BridgeBattleEvent> BattleBridge::consumeEvents() {
    std::vector<BridgeBattleEvent> bridgeEvents;
    std::vector<BattleEvent> events = battle_.consumeEvents();
    bridgeEvents.reserve(events.size());

    // 事件桥接时补上卡牌名称，方便 UI 不回查定义表就能直接播动画。
    for (const BattleEvent& event : events) {
        BridgeBattleEvent bridgeEvent;
        bridgeEvent.type = toToken(event.type);
        bridgeEvent.phase = toToken(event.phase);
        bridgeEvent.entity = toToken(event.entity);
        bridgeEvent.amount = event.amount;
        bridgeEvent.amount2 = event.amount2;
        bridgeEvent.turn = event.turn;
        bridgeEvent.cardInstanceId = event.cardInstanceId;
        bridgeEvent.cardDefId = event.cardDefId;

        const CardData* def = battle_.tryGetCardDef(event.cardDefId);
        if (def != nullptr) {
            bridgeEvent.cardId = def->id;
            bridgeEvent.cardName = def->name;
        }

        bridgeEvent.fromZone = toToken(event.fromZone);
        bridgeEvent.toZone = toToken(event.toZone);
        bridgeEvents.push_back(std::move(bridgeEvent));
    }

    return bridgeEvents;
}

std::vector<BridgeCardDefinition> BattleBridge::getCardCatalog() const {
    std::vector<BridgeCardDefinition> catalog;
    // 目录读取使用配置里的静态定义，而不是运行时状态。
    // 这样前端即使在战斗开始前，也能先生成卡牌图鉴或卡面资源。
    const std::vector<CardData>& defs = config_.cardDefs;
    catalog.reserve(defs.size());

    for (int defId = 0; defId < static_cast<int>(defs.size()); ++defId) {
        catalog.push_back(makeCardDefinition(defId, defs[defId]));
    }

    return catalog;
}

const Battle& BattleBridge::getBattle() const {
    return battle_;
}

std::string BattleBridge::toToken(EffectType type) {
    switch (type) {
    case EffectType::Damage:
        return "damage";
    case EffectType::Block:
        return "block";
    case EffectType::Draw:
        return "draw";
    }

    return "unknown";
}

std::string BattleBridge::toToken(EntityId id) {
    switch (id) {
    case EntityId::Player:
        return "player";
    case EntityId::Enemy:
        return "enemy";
    }

    return "unknown";
}

std::string BattleBridge::toToken(CardZone zone) {
    switch (zone) {
    case CardZone::None:
        return "none";
    case CardZone::DrawPile:
        return "drawpile";
    case CardZone::Hand:
        return "hand";
    case CardZone::Discard:
        return "discard";
    }

    return "unknown";
}

std::string BattleBridge::toToken(BattlePhase phase) {
    switch (phase) {
    case BattlePhase::BattleStart:
        return "battle_start";
    case BattlePhase::PlayerTurnStart:
        return "player_turn_start";
    case BattlePhase::PlayerInput:
        return "player_input";
    case BattlePhase::PlayerActing:
        return "player_acting";
    case BattlePhase::EnemyTurnStart:
        return "enemy_turn_start";
    case BattlePhase::EnemyActing:
        return "enemy_acting";
    case BattlePhase::Victory:
        return "victory";
    case BattlePhase::Defeat:
        return "defeat";
    }

    return "unknown";
}

std::string BattleBridge::toToken(BattleEventType type) {
    switch (type) {
    case BattleEventType::BattleInitialized:
        return "battle_initialized";
    case BattleEventType::PhaseChanged:
        return "phase_changed";
    case BattleEventType::TurnStarted:
        return "turn_started";
    case BattleEventType::EnergyChanged:
        return "energy_changed";
    case BattleEventType::CardMoved:
        return "card_moved";
    case BattleEventType::DeckShuffled:
        return "deck_shuffled";
    case BattleEventType::DamageApplied:
        return "damage_applied";
    case BattleEventType::BlockChanged:
        return "block_changed";
    case BattleEventType::BattleEnded:
        return "battle_ended";
    }

    return "unknown";
}

BridgeCardEffect BattleBridge::makeCardEffect(const EffectData& effect) {
    BridgeCardEffect bridgeEffect;
    bridgeEffect.type = toToken(effect.type);
    bridgeEffect.target = toToken(effect.target);
    bridgeEffect.value = effect.value;
    return bridgeEffect;
}

BridgeCardDefinition BattleBridge::makeCardDefinition(int defId, const CardData& def) const {
    BridgeCardDefinition definition;
    definition.defId = defId;
    definition.id = def.id;
    definition.name = def.name;
    definition.baseCost = def.baseCost;
    definition.effects.reserve(def.effects.size());

    for (const EffectData& effect : def.effects) {
        definition.effects.push_back(makeCardEffect(effect));
    }

    return definition;
}

BridgeCardState BattleBridge::makeCardState(const CardView& card) const {
    // 运行时卡牌状态会补齐名字、id 和效果，方便前端一次读取就够用。
    BridgeCardState state;
    state.instanceId = card.instanceId;
    state.defId = card.defId;
    state.cost = card.cost;

    const CardData* def = battle_.tryGetCardDef(card.defId);
    if (def == nullptr) {
        return state;
    }

    state.id = def->id;
    state.name = def->name;
    state.effects.reserve(def->effects.size());
    for (const EffectData& effect : def->effects) {
        state.effects.push_back(makeCardEffect(effect));
    }

    return state;
}
