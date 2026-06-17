#include <algorithm>
#include <memory>
#include <utility>

#include "battle.h"

void Battle::initialize(const BattleConfig& config) {
    // 初始化时先清空所有运行时残留，确保同一个 Battle 可以安全复用。
    pendingEvents_.clear();
    actionQueue_.clear();
    nextCardInstanceId_ = 1;

    energyCap_ = config.energyCap;
    energyMax_ = config.startingEnergy;
    energy_ = energyMax_;
    openingHandNum_ = config.openingHandNum;
    drawPerTurn_ = config.drawPerTurn;
    handLimit_ = config.handLimit;
    enemyData_ = config.enemy;

    player_ = { config.playerMaxHp, config.playerMaxHp, 0 };
    enemy_ = { enemyData_.maxHp, enemyData_.maxHp, 0 };
    turn_ = 0;

    cardDefs_ = config.cardDefs;
    hand_.clear();
    discard_.clear();
    drawpile_ = config.startingDeck;
    assignCardInstanceIds(drawpile_);

    // BattleStart 表示“战斗刚创建好，但还没真正进入玩家回合”。
    setPhase(BattlePhase::BattleStart);
    std::shuffle(drawpile_.begin(), drawpile_.end(), rng);

    BattleEvent initEvent;
    initEvent.type = BattleEventType::BattleInitialized;
    initEvent.phase = phase_;
    recordEvent(initEvent);
    recordEnergyChanged();

    drawCards(openingHandNum_);
    updateOutcome();
}

void Battle::setRandomSeed(unsigned int seed) {
    rng.seed(seed);
}

bool Battle::advance() {
    // advance 是表现层最重要的推进接口。
    // 每调一次，只向前推进一个明确步骤，方便 UI 分步播放动画。
    if (isBattleOver()) {
        return false;
    }

    if (phase_ == BattlePhase::BattleStart || phase_ == BattlePhase::PlayerTurnStart) {
        startPlayerTurn();
        return true;
    }

    if (phase_ == BattlePhase::EnemyTurnStart) {
        startEnemyTurn();
        return true;
    }

    if (!actionQueue_.empty()) {
        actionQueue_.executeNext(*this);
        updateOutcome();
        // 当动作队列清空时，需要把阶段从“行动中”切回下一个可停留阶段。
        if (!isBattleOver() && actionQueue_.empty()) {
            onActionQueueDrained();
        }
        return true;
    }

    return false;
}

void Battle::beginPlayerTurn() {
    startPlayerTurn();
}

PlayResult Battle::tryPlayCard(int handIndex) {
    if (!canAcceptInput()) {
        return PlayResult::InvalidPhase;
    }

    if (handIndex < 0 || handIndex >= static_cast<int>(hand_.size())) {
        return PlayResult::InvalidIndex;
    }

    return tryPlayCardByInstanceId(hand_[handIndex].instanceId);
}

PlayResult Battle::tryPlayCardByInstanceId(int instanceId) {
    if (!canAcceptInput()) {
        return PlayResult::InvalidPhase;
    }

    // 表现层推荐使用 instanceId 出牌，这样不会因为手牌重排而点错牌。
    const int handIndex = findHandIndexByInstanceId(instanceId);
    if (handIndex < 0) {
        return PlayResult::InvalidIndex;
    }

    const CardInstance card = hand_[handIndex];
    if (card.defId < 0 || card.defId >= static_cast<int>(cardDefs_.size())) {
        return PlayResult::InvalidIndex;
    }

    const int totalCost = computeCardCost(card);
    if (energy_ < totalCost) {
        return PlayResult::NotEnoughEnergy;
    }

    energy_ -= totalCost;
    recordEnergyChanged();

    // 当前实现里，打出的牌会立即离开手牌并进入弃牌堆。
    discard_.push_back(card);
    hand_.erase(hand_.begin() + handIndex);

    BattleEvent moveEvent;
    moveEvent.type = BattleEventType::CardMoved;
    moveEvent.phase = phase_;
    moveEvent.turn = turn_;
    moveEvent.cardInstanceId = card.instanceId;
    moveEvent.cardDefId = card.defId;
    moveEvent.fromZone = CardZone::Hand;
    moveEvent.toZone = CardZone::Discard;
    recordEvent(moveEvent);

    queueAction(std::make_unique<PlayCardAction>(card));
    // 进入 PlayerActing，表示“这次出牌的后续动作正在结算中”。
    setPhase(BattlePhase::PlayerActing);
    updateOutcome();

    return PlayResult::Ok;
}

void Battle::endPlayerTurn() {
    if (!canAcceptInput()) {
        return;
    }

    // 当前规则：结束回合时剩余手牌全部弃掉。
    discardHand();
    setPhase(BattlePhase::EnemyTurnStart);
    updateOutcome();
}

void Battle::queueEnemyTurn() {
    startEnemyTurn();
}

void Battle::resolveActions() {
    while (!actionQueue_.empty() && !isBattleOver()) {
        advance();
    }
}

bool Battle::canAcceptInput() const {
    return phase_ == BattlePhase::PlayerInput && actionQueue_.empty() && !isBattleOver();
}

bool Battle::isBattleOver() const {
    return phase_ == BattlePhase::Victory || phase_ == BattlePhase::Defeat;
}

bool Battle::hasPendingActions() const {
    return !actionQueue_.empty();
}

int Battle::computeCardCost(const CardInstance& card) const {
    if (card.defId < 0 || card.defId >= static_cast<int>(cardDefs_.size())) {
        return 0;
    }

    const CardData& def = cardDefs_[card.defId];
    return std::max(0, def.baseCost + card.costDelta);
}

int Battle::getTurn() const {
    return turn_;
}

BattlePhase Battle::getPhase() const {
    return phase_;
}

const Entity& Battle::getEntity(EntityId id) const {
    return id == EntityId::Player ? player_ : enemy_;
}

const EnemyData& Battle::getEnemyData() const {
    return enemyData_;
}

const std::vector<CardData>& Battle::getCardDefs() const {
    return cardDefs_;
}

const CardData* Battle::tryGetCardDef(int defId) const {
    if (defId < 0 || defId >= static_cast<int>(cardDefs_.size())) {
        return nullptr;
    }

    return &cardDefs_[defId];
}

const std::vector<CardInstance>& Battle::getHand() const {
    return hand_;
}

BattleSnapshot Battle::getSnapshot() const {
    // 快照适合用来“整帧刷新 UI”。
    BattleSnapshot snapshot;
    snapshot.phase = phase_;
    snapshot.player = player_;
    snapshot.enemy = enemy_;
    snapshot.turn = turn_;
    snapshot.energy = energy_;
    snapshot.energyCap = energyCap_;
    snapshot.energyMax = energyMax_;
    snapshot.enemyData = enemyData_;
    snapshot.enemyIntent = getEnemyIntent();
    snapshot.drawpileCount = static_cast<int>(drawpile_.size());
    snapshot.discardCount = static_cast<int>(discard_.size());
    snapshot.hasPendingActions = !actionQueue_.empty();

    snapshot.hand.reserve(hand_.size());
    for (const CardInstance& card : hand_) {
        snapshot.hand.push_back(makeCardView(card));
    }

    return snapshot;
}

EnemyIntent Battle::getEnemyIntent() const {
    EnemyIntent intent;
    if (isBattleOver()) {
        return intent;
    }

    // 目前敌人行为还很简单，所以意图直接映射为固定伤害。
    intent.known = true;
    intent.type = EffectType::Damage;
    intent.target = EntityId::Player;
    intent.value = enemyData_.attackDamage;
    return intent;
}

std::vector<BattleEvent> Battle::consumeEvents() {
    // 事件一旦被消费就会清空，表现层应当及时取走。
    std::vector<BattleEvent> events = std::move(pendingEvents_);
    pendingEvents_.clear();
    return events;
}

int Battle::drawCards(int n) {
    int actualDrawn = 0;
    for (int i = 0; i < n; ++i) {
        if (hand_.size() >= static_cast<size_t>(handLimit_)) {
            break;
        }

        reshuffleIfNeeded();
        if (drawpile_.empty()) {
            break;
        }

        CardInstance card = drawpile_.back();
        drawpile_.pop_back();
        hand_.push_back(card);
        ++actualDrawn;

        // 每张牌的移动都单独记事件，方便前端逐张播放抽牌动画。
        BattleEvent moveEvent;
        moveEvent.type = BattleEventType::CardMoved;
        moveEvent.phase = phase_;
        moveEvent.turn = turn_;
        moveEvent.cardInstanceId = card.instanceId;
        moveEvent.cardDefId = card.defId;
        moveEvent.fromZone = CardZone::DrawPile;
        moveEvent.toZone = CardZone::Hand;
        recordEvent(moveEvent);
    }

    return actualDrawn;
}

void Battle::discardHand() {
    for (const CardInstance& card : hand_) {
        discard_.push_back(card);

        BattleEvent moveEvent;
        moveEvent.type = BattleEventType::CardMoved;
        moveEvent.phase = phase_;
        moveEvent.turn = turn_;
        moveEvent.cardInstanceId = card.instanceId;
        moveEvent.cardDefId = card.defId;
        moveEvent.fromZone = CardZone::Hand;
        moveEvent.toZone = CardZone::Discard;
        recordEvent(moveEvent);
    }
    hand_.clear();
}

void Battle::queueAction(std::unique_ptr<GameAction> action) {
    actionQueue_.push(std::move(action));
}

void Battle::expandCardEffects(const CardInstance& card) {
    if (card.defId < 0 || card.defId >= static_cast<int>(cardDefs_.size())) {
        return;
    }

    // 这里按牌面顺序把效果压入动作队列，后续由动作系统逐个执行。
    const CardData& cardDef = cardDefs_[card.defId];
    for (const EffectData& effect : cardDef.effects) {
        enqueueEffect(effect);
    }
}

void Battle::enqueueEffect(const EffectData& effect) {
    switch (effect.type) {
    case EffectType::Damage:
        queueAction(std::make_unique<DamageAction>(effect.target, effect.value));
        break;
    case EffectType::Block:
        queueAction(std::make_unique<GainBlockAction>(effect.target, effect.value));
        break;
    case EffectType::Draw:
        queueAction(std::make_unique<DrawCardsAction>(effect.value));
        break;
    }
}

void Battle::updateOutcome() {
    // 战斗胜负统一在这里收口，避免不同动作各自重复判断。
    if (enemy_.hp <= 0) {
        enemy_.hp = 0;
        if (phase_ != BattlePhase::Victory) {
            setPhase(BattlePhase::Victory);

            BattleEvent endEvent;
            endEvent.type = BattleEventType::BattleEnded;
            endEvent.phase = phase_;
            endEvent.turn = turn_;
            endEvent.entity = EntityId::Enemy;
            recordEvent(endEvent);
        }
    } else if (player_.hp <= 0) {
        player_.hp = 0;
        if (phase_ != BattlePhase::Defeat) {
            setPhase(BattlePhase::Defeat);

            BattleEvent endEvent;
            endEvent.type = BattleEventType::BattleEnded;
            endEvent.phase = phase_;
            endEvent.turn = turn_;
            endEvent.entity = EntityId::Player;
            recordEvent(endEvent);
        }
    }
}

void Battle::applyDamage(EntityId targetId, int damage) {
    Entity& target = targetId == EntityId::Player ? player_ : enemy_;
    const int blocked = std::min(target.block, std::max(0, damage));
    const int applied = std::max(0, damage - blocked);

    // 先扣格挡，再扣生命，事件里同时记录两部分结果。
    if (blocked != 0) {
        target.block -= blocked;
        recordBlockChanged(targetId, -blocked, target.block);
    }

    if (applied != 0) {
        target.hp = std::max(0, target.hp - applied);
    }

    BattleEvent damageEvent;
    damageEvent.type = BattleEventType::DamageApplied;
    damageEvent.phase = phase_;
    damageEvent.entity = targetId;
    damageEvent.amount = applied;
    damageEvent.amount2 = blocked;
    damageEvent.turn = turn_;
    recordEvent(damageEvent);
}

void Battle::gainBlock(EntityId targetId, int block) {
    if (block == 0) {
        return;
    }

    Entity& target = targetId == EntityId::Player ? player_ : enemy_;
    target.block += block;
    recordBlockChanged(targetId, block, target.block);
}

void Battle::setPhase(BattlePhase phase) {
    if (phase_ == phase) {
        return;
    }

    phase_ = phase;

    BattleEvent phaseEvent;
    phaseEvent.type = BattleEventType::PhaseChanged;
    phaseEvent.phase = phase_;
    phaseEvent.turn = turn_;
    recordEvent(phaseEvent);
}

void Battle::recordEvent(BattleEvent event) {
    pendingEvents_.push_back(std::move(event));
}

void Battle::recordEnergyChanged() {
    BattleEvent energyEvent;
    energyEvent.type = BattleEventType::EnergyChanged;
    energyEvent.phase = phase_;
    energyEvent.amount = energy_;
    energyEvent.amount2 = energyMax_;
    energyEvent.turn = turn_;
    recordEvent(energyEvent);
}

void Battle::recordBlockChanged(EntityId id, int delta, int newBlock) {
    BattleEvent blockEvent;
    blockEvent.type = BattleEventType::BlockChanged;
    blockEvent.phase = phase_;
    blockEvent.entity = id;
    blockEvent.amount = delta;
    blockEvent.amount2 = newBlock;
    blockEvent.turn = turn_;
    recordEvent(blockEvent);
}

void Battle::startPlayerTurn() {
    if (isBattleOver()) {
        return;
    }

    // 玩家回合开始时，phase 会先进入 TurnStart，再切回可输入状态。
    setPhase(BattlePhase::PlayerTurnStart);
    ++turn_;

    BattleEvent turnEvent;
    turnEvent.type = BattleEventType::TurnStarted;
    turnEvent.phase = phase_;
    turnEvent.turn = turn_;
    recordEvent(turnEvent);

    if (player_.block != 0) {
        const int lostBlock = -player_.block;
        player_.block = 0;
        recordBlockChanged(EntityId::Player, lostBlock, player_.block);
    }

    if (energyMax_ < energyCap_ && turn_ > 1) {
        ++energyMax_;
    }

    energy_ = energyMax_;
    recordEnergyChanged();

    if (turn_ > 1) {
        drawCards(drawPerTurn_);
    }

    // 到这里玩家回合的自动步骤已经完成，可以开始接收输入。
    setPhase(BattlePhase::PlayerInput);
    updateOutcome();
}

void Battle::startEnemyTurn() {
    if (isBattleOver()) {
        return;
    }

    setPhase(BattlePhase::EnemyTurnStart);

    if (enemy_.block != 0) {
        const int lostBlock = -enemy_.block;
        enemy_.block = 0;
        recordBlockChanged(EntityId::Enemy, lostBlock, enemy_.block);
    }

    // 敌人回合目前只会压入一个基础攻击动作，后面可以在这里扩展 AI。
    queueAction(std::make_unique<EnemyAttackAction>(enemyData_.attackDamage));
    setPhase(BattlePhase::EnemyActing);
}

void Battle::onActionQueueDrained() {
    // 当“行动中”的动作都结算完后，切回能停下来的阶段。
    if (phase_ == BattlePhase::PlayerActing) {
        setPhase(BattlePhase::PlayerInput);
    } else if (phase_ == BattlePhase::EnemyActing) {
        setPhase(BattlePhase::PlayerTurnStart);
    }
}

CardView Battle::makeCardView(const CardInstance& card) const {
    CardView view;
    view.instanceId = card.instanceId;
    view.defId = card.defId;
    view.cost = computeCardCost(card);
    return view;
}

int Battle::findHandIndexByInstanceId(int instanceId) const {
    for (int i = 0; i < static_cast<int>(hand_.size()); ++i) {
        if (hand_[i].instanceId == instanceId) {
            return i;
        }
    }

    return -1;
}

void Battle::assignCardInstanceIds(std::vector<CardInstance>& cards) {
    // 每张起始卡牌在开局时获得唯一 ID，供表现层后续稳定跟踪。
    for (CardInstance& card : cards) {
        card.instanceId = nextCardInstanceId_++;
    }
}

void Battle::reshuffleIfNeeded() {
    if (!drawpile_.empty() || discard_.empty()) {
        return;
    }

    // 抽牌堆空了以后，把弃牌堆洗回抽牌堆。
    drawpile_ = discard_;
    discard_.clear();
    std::shuffle(drawpile_.begin(), drawpile_.end(), rng);

    BattleEvent shuffleEvent;
    shuffleEvent.type = BattleEventType::DeckShuffled;
    shuffleEvent.phase = phase_;
    shuffleEvent.turn = turn_;
    shuffleEvent.amount = static_cast<int>(drawpile_.size());
    recordEvent(shuffleEvent);
}
