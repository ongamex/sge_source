#pragma once

#include <chrono>
#include <vector>

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/Random.h"
#include "sge_utils/math/color.h"
#include "sge_utils/math/vec4.h"
#include "sge_utils/utils/Event.h"
#include "sge_utils/utils/optional.h"

#include "sge_renderer/renderer/renderer.h"

#include "Layout.h"
#include "Sizing.h"

#include <atomic>

namespace sge {

struct QuickDraw;
struct InputState;
struct DebugFont;

} // namespace sge

namespace sge::gamegui {

struct UIDrawSets;
struct UIContext;
struct ILayout;

/// @brief IWidget is the base interface for all widget.
struct SGE_CORE_API IWidget : public std::enable_shared_from_this<IWidget> {
	friend UIContext;

	IWidget(UIContext& owningContext)
	    : m_owningContext(owningContext) {}
	virtual ~IWidget() {}

	virtual bool isGamepadTargetable() { return false; }

	virtual bool onHoverEnter() { return false; }
	virtual bool onHoverLeave() { return false; }
	virtual bool onPress() { return false; }
	virtual bool onMouseWheel(int UNUSED(cnt)) { return false; }

	/// Called only if the press happend.
	virtual void onRelease(bool UNUSED(wasReleaseInside)) {}

	virtual void draw(const UIDrawSets& drawSets) = 0;
	virtual void update() {}

	Pos getPosition() const { return m_position; }
	Pos& getPosition() { return m_position; }

	void setPosition(Pos newPos) { m_position = newPos; }

	Size getSize() const { return m_size; }
	Size& getSize() { return m_size; }

	void setSize(Size newSize) { m_size = newSize; }

	AABox2f getBBoxPixels() const {
		AABox2f parentBBoxSS = getParentBBoxSS();
		const AABox2f bboxSS = m_position.getBBoxPixels(parentBBoxSS, getParentContentOrigin().toPixels(parentBBoxSS.size()), m_size);
		return bboxSS;
	}

	AABox2f getScissorBoxSS() const;
	Rect2s getScissorRect() const;

	void addChild(std::shared_ptr<IWidget> widget);

	const std::vector<std::shared_ptr<IWidget>>& getChildren() const { return m_children; }

	void setLayout(ILayout* layout);

	ILayout* getLayout();
	const ILayout* getLayout() const;

	UIContext& getContext() { return m_owningContext; }
	const UIContext& getContext() const { return m_owningContext; }

	void suspend() { m_isSuspended = true; }
	void unsuspend() { m_isSuspended = false; }
	bool isSuspended() const {
		if (m_isSuspended) {
			return true;
		}

		auto parent = getParent();
		return (parent && parent->isSuspended());
	}

	std::shared_ptr<IWidget> getParent() { return m_parentWidget.lock(); }

	std::shared_ptr<IWidget> getParent() const { return m_parentWidget.lock(); }

	const Pos& getContentsOrigin() const { return m_contentsOrigin; }
	void setContentsOrigin(const Pos& o) { m_contentsOrigin = o; }

  protected:
	AABox2f getParentBBoxSS() const;
	Pos getParentContentOrigin() const;

  private:
	Pos m_position;
	Size m_size;
	bool m_isSuspended = false;
	Pos m_contentsOrigin;

	bool m_wasHoveredPrevFrame = false;

	UIContext& m_owningContext;
	std::weak_ptr<IWidget> m_parentWidget;
	std::vector<std::shared_ptr<IWidget>> m_children;
	std::unique_ptr<ILayout> m_layout = nullptr;
};


/// @brief InvisibleWidget is just an invisible widget with size and position
/// useful for position and aligning objects or root widget for menus.
struct SGE_CORE_API InvisibleWidget final : public IWidget {
	InvisibleWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext) {
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<InvisibleWidget> create(UIContext& owningContext, Pos position, Size size) {
		std::shared_ptr<InvisibleWidget> w = std::make_shared<InvisibleWidget>(owningContext, position, size);
		return w;
	}

	virtual void draw(const UIDrawSets& UNUSED(drawSets)) override {}
};

/// @brief ColoredWidget is non-interactable widget with a constant color.
struct SGE_CORE_API ColoredWidget final : public IWidget {
	ColoredWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext) {
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<ColoredWidget> create(UIContext& owningContext, Pos position, Size size);

	virtual void draw(const UIDrawSets& drawSets) override;

	void setColor(const vec4f& c) { m_color = c; }

	bool onMouseWheel(int cnt) override {
		Pos co = getContentsOrigin().getAsFraction(getParentBBoxSS().size());
		co.posY.value -= float(cnt) * 0.2f;
		setContentsOrigin(co);
		return true;
	}

  private:
	vec4f m_color = vec4f(1.f);
};

//----------------------------------------------------
// TextWidget
//----------------------------------------------------
struct SGE_CORE_API TextWidget final : public IWidget {
	TextWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext) {
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<TextWidget> create(UIContext& owningContext, Pos position, Size size, const char* text) {
		auto w = std::make_shared<TextWidget>(owningContext, position, size);
		if (text) {
			w->setText(text);
		}
		return w;
	}

	virtual void draw(const UIDrawSets& drawSets) override;

	void setColor(const vec4f& c) { m_color = c; }
	void setText(std::string s) { m_text = std::move(s); }
	void setFont(DebugFont* font) { m_font = font; }
	void setFontSize(Unit fontSize) { m_fontSize = fontSize; }

  private:
	DebugFont* m_font = nullptr;
	vec4f m_color = vec4f(1.f);
	Unit m_fontSize = Unit::fromFrac(1.f);
	std::string m_text;
};

/// ImageWidget is a widget that display a texture.
struct SGE_CORE_API ImageWidget final : public IWidget {
	ImageWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext) {
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<ImageWidget> create(UIContext& owningContext, Pos position, Unit width, GpuHandle<Texture> texture);
	virtual void draw(const UIDrawSets& drawSets) override;

  private:
	GpuHandle<Texture> m_texture;
	vec4f uvRegion = vec4f(0.f, 0.f, 1.f, 1.f);
};

//----------------------------------------------------
// ButtonWidget
//----------------------------------------------------
struct SGE_CORE_API ButtonWidget final : public IWidget {
	ButtonWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext) {
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<ButtonWidget> create(UIContext& owningContext, Pos position, Size size, const char* const text = nullptr);

	bool isGamepadTargetable() override { return true; }

	void draw(const UIDrawSets& drawSets) override;

	void setColor(const vec4f& c) { m_textColor = c; }
	void setBgColor(const vec4f& up, const vec4f& hovered, const vec4f& pressed) {
		m_bgColorUp = up;
		m_bgColorHovered = hovered;
		m_bgColorPressed = pressed;
	}
	void setText(std::string s) { m_text = std::move(s); }
	void setArrow(SignedAxis axis) { m_triangleDir = axis; }
	void setFont(DebugFont* font) { m_font = font; }
	void setFontSize(Unit fontSize) { m_fontSize = fontSize; }

	bool onHoverEnter() override {
		m_isHovered = true;
		return true;
	}

	bool onHoverLeave() override {
		m_isHovered = false;
		return true;
	}

	bool onPress() override {
		m_isPressed = true;
		return true;
	}

	void onRelease(bool wasReleaseInside) override {
		m_isPressed = false;
		if (wasReleaseInside) {
			m_onReleaseListeners.invokeEvent();
		}
	}

	/// @brief Adds a function to get called when the button has been released (cursor over the widget) or when interacted with the gamepad.
	[[nodiscard]] EventSubscription subscribe_onRelease(std::function<void()> fn) { return m_onReleaseListeners.subscribe(std::move(fn)); }

  private:
	EventEmitter<> m_onReleaseListeners;

	bool m_isHovered = false;
	bool m_isPressed = false;

	DebugFont* m_font = nullptr;
	vec4f m_textColor = vec4f(1.f);
	vec4f m_bgColorUp = colorWhite(0.33f);
	vec4f m_bgColorHovered = colorWhite(0.66f);
	vec4f m_bgColorPressed = colorWhite(0.22f);
	Unit m_fontSize = Unit::fromFrac(1.f);
	std::string m_text;
	SignedAxis m_triangleDir = SignedAxis::signedAxis_numElements;
};

//----------------------------------------------------
// HorizontalComboBox
//----------------------------------------------------
struct SGE_CORE_API HorizontalComboBox final : public IWidget {
	HorizontalComboBox(UIContext& owningContext, Pos position, Size size);

	static std::shared_ptr<HorizontalComboBox> create(UIContext& owningContext, Pos position, Size size);
	void draw(const UIDrawSets& drawSets) override;

	void addOption(std::string option) {
		m_options.emplace_back(std::move(option));
		if (m_currentOption < 0) {
			m_currentOption = 0;
		}
	}

  private:
	void leftPressed();
	void rightPressed();

	EventSubscription m_leftPressSub;
	EventSubscription m_rightPressSub;

	std::weak_ptr<ButtonWidget> m_leftBtn;
	std::weak_ptr<ButtonWidget> m_rightBtn;

	int m_currentOption = -1;
	std::vector<std::string> m_options;
};

//----------------------------------------------------
// Checkbox
//----------------------------------------------------
struct SGE_CORE_API Checkbox final : public IWidget {
	Checkbox(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext) {
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<Checkbox> create(UIContext& owningContext, Pos position, Size size, const char* const text, bool isOn);

	bool isGamepadTargetable() override { return true; }

	void draw(const UIDrawSets& drawSets) override;

	bool onPress() override;
	void onRelease(bool wasReleaseInside) override;

	void setText(std::string s) { m_text = std::move(s); }

	EventSubscription onRelease(std::function<void(bool)> fn) { return m_onReleaseListeners.subscribe(std::move(fn)); }

  private:
	bool m_isOn = false;
	bool m_isPressed = false;
	std::string m_text;

	EventEmitter<bool> m_onReleaseListeners;
};

} // namespace sge::gamegui
