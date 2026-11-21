#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

using namespace geode::prelude;

struct IconSpec {
    std::string prompt;
    bool pointy = false;
    bool wave = false;
    bool round = false;
    float width = 1.f;
    float height = 1.f;
    ccColor4F color = {0.2f, 0.9f, 1.f, 1.f};
};

static bool has(std::string const& s, char const* word) {
    return s.find(word) != std::string::npos;
}

static IconSpec parseSpec(std::string prompt) {
    IconSpec spec;
    spec.prompt = prompt;
    std::string p = prompt;
    std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c) { return std::tolower(c); });

    if (has(p, "point") || has(p, "sharp") || has(p, "spike")) spec.pointy = true;
    if (has(p, "wave") || has(p, "dart")) spec.wave = true;
    if (has(p, "round") || has(p, "circle") || has(p, "ball")) spec.round = true;
    if (has(p, "elongat") || has(p, "long") || has(p, "stretch")) spec.width = 1.8f;
    if (has(p, "tall")) spec.height = 1.6f;
    if (has(p, "short") || has(p, "tiny") || has(p, "small")) { spec.width *= 0.6f; spec.height *= 0.6f; }

    if (has(p, "red")) spec.color = {1.f, 0.2f, 0.2f, 1.f};
    else if (has(p, "green")) spec.color = {0.2f, 1.f, 0.3f, 1.f};
    else if (has(p, "blue")) spec.color = {0.2f, 0.4f, 1.f, 1.f};
    else if (has(p, "pink") || has(p, "magenta")) spec.color = {1.f, 0.2f, 0.8f, 1.f};
    else if (has(p, "yellow") || has(p, "gold")) spec.color = {1.f, 0.9f, 0.2f, 1.f};
    else if (has(p, "purple")) spec.color = {0.6f, 0.2f, 1.f, 1.f};
    else if (has(p, "white")) spec.color = {1.f, 1.f, 1.f, 1.f};

    return spec;
}

static CCNode* makeIconNode(IconSpec const& spec, float scale) {
    auto draw = CCDrawNode::create();
    std::vector<CCPoint> pts;

    if (spec.round) {
        for (int i = 0; i < 24; i++) {
            float a = (float)i / 24.f * 6.2831853f;
            pts.push_back({std::cos(a) * 22.f, std::sin(a) * 22.f});
        }
    }
    else if (spec.wave) {
        pts = {{-30, -14}, {32, 0}, {-30, 14}, {-14, 0}};
    }
    else if (spec.pointy) {
        pts = {{-20, -20}, {32, 0}, {-20, 20}};
    }
    else {
        pts = {{-20, -20}, {20, -20}, {20, 20}, {-20, 20}};
    }

    for (auto& pt : pts) {
        pt.x *= spec.width * scale;
        pt.y *= spec.height * scale;
    }

    ccColor4F border = {1.f, 1.f, 1.f, 1.f};
    draw->drawPolygon(pts.data(), (unsigned int)pts.size(), spec.color, 1.5f, border);
    return draw;
}

class IconAIPopup : public Popup<> {
protected:
    TextInput* m_input = nullptr;
    CCLabelBMFont* m_chat = nullptr;
    CCNode* m_previewHolder = nullptr;
    CCMenuItemSpriteExtra* m_sureBtn = nullptr;
    CCMenuItemSpriteExtra* m_noBtn = nullptr;
    CCMenu* m_historyMenu = nullptr;
    std::vector<IconSpec> m_history;
    IconSpec m_current;
    bool m_hasCurrent = false;

    bool setup() override {
        this->setTitle("Icon Maker");
        auto size = m_mainLayer->getContentSize();

        m_chat = CCLabelBMFont::create("Describe the icon you want!", "bigFont.fnt");
        m_chat->setScale(0.45f);
        m_chat->setPosition({size.width / 2, size.height - 45.f});
        m_mainLayer->addChild(m_chat);

        m_input = TextInput::create(250.f, "e.g. pointy elongated wave", "bigFont.fnt");
        m_input->setPosition({size.width / 2 - 35.f, size.height - 75.f});
        m_mainLayer->addChild(m_input);

        auto genBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Go", 40, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.6f),
            this, menu_selector(IconAIPopup::onGenerate)
        );
        genBtn->setPosition({size.width / 2 + 125.f, size.height - 75.f});
        m_buttonMenu->addChild(genBtn);

        m_previewHolder = CCNode::create();
        m_previewHolder->setPosition({size.width / 2, size.height - 125.f});
        m_mainLayer->addChild(m_previewHolder);

        m_sureBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Sure", 60, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.6f),
            this, menu_selector(IconAIPopup::onSure)
        );
        m_sureBtn->setPosition({size.width / 2 - 70.f, size.height - 170.f});
        m_sureBtn->setVisible(false);
        m_buttonMenu->addChild(m_sureBtn);

        m_noBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("No, retry", 90, true, "goldFont.fnt", "GJ_button_06.png", 30.f, 0.6f),
            this, menu_selector(IconAIPopup::onNo)
        );
        m_noBtn->setPosition({size.width / 2 + 60.f, size.height - 170.f});
        m_noBtn->setVisible(false);
        m_buttonMenu->addChild(m_noBtn);

        auto histLabel = CCLabelBMFont::create("Your icons (tap one to use it)", "chatFont.fnt");
        histLabel->setScale(0.7f);
        histLabel->setPosition({size.width / 2, 45.f});
        m_mainLayer->addChild(histLabel);

        m_historyMenu = CCMenu::create();
        m_historyMenu->setPosition({0.f, 0.f});
        m_historyMenu->setContentSize(size);
        m_mainLayer->addChild(m_historyMenu);

        return true;
    }

    void onGenerate(CCObject*) {
        std::string text = m_input->getString();
        if (text.empty()) {
            m_chat->setString("Type a description first!");
            return;
        }
        m_current = parseSpec(text);
        m_hasCurrent = true;

        m_previewHolder->removeAllChildren();
        m_previewHolder->addChild(makeIconNode(m_current, 1.f));

        m_chat->setString("This good?");
        m_sureBtn->setVisible(true);
        m_noBtn->setVisible(true);
    }

    void onSure(CCObject*) {
        if (!m_hasCurrent) return;
        m_history.push_back(m_current);
        m_hasCurrent = false;
        m_sureBtn->setVisible(false);
        m_noBtn->setVisible(false);
        m_chat->setString("Saved! Make another or pick one below.");
        this->refreshHistory();
    }

    void onNo(CCObject*) {
        m_hasCurrent = false;
        m_sureBtn->setVisible(false);
        m_noBtn->setVisible(false);
        m_previewHolder->removeAllChildren();
        m_input->setString("");
        m_chat->setString("Okay, describe what you want instead.");
    }

    void refreshHistory() {
        m_historyMenu->removeAllChildren();
        auto size = m_mainLayer->getContentSize();
        int count = (int)m_history.size();
        for (int i = 0; i < count; i++) {
            auto node = makeIconNode(m_history[i], 0.45f);
            auto holder = CCNode::create();
            holder->setContentSize({46.f, 36.f});
            node->setPosition({23.f, 18.f});
            holder->addChild(node);

            auto btn = CCMenuItemSpriteExtra::create(holder, this, menu_selector(IconAIPopup::onPick));
            btn->setTag(i);
            float spacing = 50.f;
            float startX = size.width / 2 - (count - 1) * spacing / 2.f;
            btn->setPosition({startX + i * spacing, 22.f});
            m_historyMenu->addChild(btn);
        }
    }

    void onPick(CCObject* sender) {
        int idx = static_cast<CCNode*>(sender)->getTag();
        if (idx < 0 || idx >= (int)m_history.size()) return;
        std::string prompt = m_history[idx].prompt;
        createQuickPopup(
            "Use this icon?",
            "Are you sure you want this?",
            "No", "Yes",
            [prompt](auto, bool yes) {
                if (yes) {
                    Mod::get()->setSavedValue<std::string>("chosen-icon-prompt", prompt);
                    Notification::create("Icon chosen!", NotificationIcon::Success)->show();
                }
            }
        );
    }

public:
    static IconAIPopup* create() {
        auto ret = new IconAIPopup();
        if (ret->initAnchored(360.f, 260.f)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class $modify(IconAIMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Icon AI"),
            this, menu_selector(IconAIMenuLayer::onIconAI)
        );
        btn->setID("icon-ai-button"_spr);

        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        return true;
    }

    void onIconAI(CCObject*) {
        IconAIPopup::create()->show();
    }
};
