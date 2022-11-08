#include "Menu.h"

namespace gui
{
	Just getJust(const String& t)
	{
		if (t == "left") return Just::left;
		if (t == "topLeft") return Just::topLeft;
		if (t == "topRight") return Just::topRight;
		if (t == "top") return Just::top;
		if (t == "bottom") return Just::bottom;
		if (t == "right") return Just::right;

		return Just::centred;
	}

	// COLOURSELECTOR

	ColourSelector::ColourSelector(Utils& u) :
		Comp(u, "", CursorType::Default),
		selector(!CS::showAlphaChannel | CS::showColourAtTop | !CS::showSliders | CS::showColourspace, 4, 7),
		revert(u, "Click here to revert to the last state of your coloursheme."),
		deflt(u, "Click here to set the coloursheme back to its default state."),
		curSheme(Colours::c(ColourID::Interact))
	{
		layout.init(
			{ 1, 1 },
			{ 8, 2 }
		);

		selector.setCurrentColour(curSheme, juce::NotificationType::dontSendNotification);

		makeTextButton(revert, "Revert", false, true);
		makeTextButton(deflt, "Default", false, true);

		revert.onClick.push_back([this](Button&, const Mouse&)
			{
				Colours::c.set(curSheme);

				selector.setCurrentColour(
					curSheme,
					juce::NotificationType::dontSendNotification
				);

				notify(EvtType::ColourSchemeChanged);
			});
		deflt.onClick.push_back([this](Button&, const Mouse&)
			{
				Colours::c.set(Colours::c.defaultColour());

				curSheme = Colours::c(ColourID::Interact);
				selector.setCurrentColour(
					curSheme,
					juce::NotificationType::dontSendNotification
				);

				notify(EvtType::ColourSchemeChanged);
			});

		addAndMakeVisible(selector);
		addAndMakeVisible(revert);
		addAndMakeVisible(deflt);

		startTimerHz(12);
	}

	void ColourSelector::paint(Graphics&){}

	void ColourSelector::resized()
	{
		layout.resized();

		layout.place(selector, 0, 0, 2, 1, false);
		layout.place(revert, 0, 1, 1, 1, false);
		layout.place(deflt, 1, 1, 1, 1, false);
	}

	void ColourSelector::timerCallback()
	{
		const auto curCol = selector.getCurrentColour();
		const auto lastCol = Colours::c(ColourID::Interact);

		if (curCol == lastCol)
			return;

		Colours::c.set(curCol);
		notify(EvtType::ColourSchemeChanged);
	}

	// ErkenntnisseComp

	ErkenntnisseComp::ErkenntnisseComp(Utils& u) :
		Comp(u, "", CursorType::Default),
		Timer(),
		editor(u, "Enter or edit wisdom.", "Enter wisdom..."),
		date(u, ""),
		manifest(u, "Click here to manifest wisdom to the manifest of wisdom!"),
		inspire(u, "Click here to get inspired by past wisdom of the manifest of wisdom!"),
		reveal(u, "Click here to reveal wisdom from the manifest of wisdom!"),
		clear(u, "Click here to clear the wisdom editor to write more wisdom!"),
		paste(u, "Click here to paste wisdom from the clipboard to the wisdom editor!")
	{
		const File folder(getFolder());
		if (!folder.exists())
			folder.createDirectory();

		layout.init
		(
			{ 1, 1, 1, 1, 1 },
			{ 8, 1, 1 }
		);

		addAndMakeVisible(editor);
		addAndMakeVisible(date);

		date.mode = Label::Mode::TextToLabelBounds;
		manifest.getLabel().mode = date.mode;
		inspire.getLabel().mode = date.mode;
		reveal.getLabel().mode = date.mode;
		clear.getLabel().mode = date.mode;
		paste.getLabel().mode = date.mode;

		addAndMakeVisible(manifest);
		addAndMakeVisible(inspire);
		addAndMakeVisible(reveal);
		addAndMakeVisible(clear);
		addAndMakeVisible(paste);

		makeTextButton(manifest, "Manifest");
		makeTextButton(inspire, "Inspire");
		makeTextButton(reveal, "Reveal");
		makeTextButton(clear, "Clear");
		makeTextButton(paste, "Paste");

		editor.onReturn = [&]()
		{
			saveToDisk();
			return true;
		};

		editor.onClick = [&]()
		{
			editor.enable();
			return true;
		};

		manifest.onClick.push_back([&](Button&, const Mouse&)
		{
			saveToDisk();
		});

		inspire.onClick.push_back([&](Button&, const Mouse&)
		{
			const File folder(getFolder());

			const auto fileTypes = File::TypesOfFileToFind::findFiles;
			const String extension(".txt");
			const auto wildCard = "*" + extension;
			const auto numFiles = folder.getNumberOfChildFiles(fileTypes, wildCard);
			if (numFiles == 0)
				return parse("I am deeply sorry. There is no wisdom in the manifest of wisdom yet.");

			Random rand;
			auto idx = rand.nextInt(numFiles);

			const RangedDirectoryIterator files
			(
				folder,
				false,
				wildCard,
				fileTypes
			);

			for (const auto& it : files)
			{
				if (idx == 0)
				{
					const File file(it.getFile());
					parse(file.getFileName());
					editor.setText(file.loadFileAsString());
					editor.disable();
					return;
				}
				else
					--idx;
			}
		});

		reveal.onClick.push_back([&](Button&, const Mouse&)
		{
			const File file(getFolder() + date.getText());
			if (file.exists())
				file.revealToUser();

			const File folder(getFolder());
			folder.revealToUser();
		});

		clear.onClick.push_back([&](Button&, const Mouse&)
		{
			editor.clear();
			editor.enable();
			parse("");
		});

		paste.onClick.push_back([&](Button&, const Mouse&)
		{
			auto cbTxt = SystemClipboard::getTextFromClipboard();
			if (cbTxt.isEmpty())
				return;
			editor.addText(editor.getText() + cbTxt);
		});

		startTimerHz(4);
	}

	void ErkenntnisseComp::timerCallback()
	{
		if (editor.isShowing())
		{
			editor.enable();
			stopTimer();
		}
	}

	void ErkenntnisseComp::resized()
	{
		layout.resized();

		layout.place(editor, 0, 0, 5, 1, false);
		layout.place(date, 0, 1, 5, 1, false);

		layout.place(manifest, 0, 2, 1, 1, false);
		layout.place(inspire, 1, 2, 1, 1, false);
		layout.place(reveal, 2, 2, 1, 1, false);
		layout.place(clear, 3, 2, 1, 1, false);
		layout.place(paste, 4, 2, 1, 1, false);
	}

	void ErkenntnisseComp::paint(Graphics&)
	{}

	String ErkenntnisseComp::getFolder()
	{
		const auto slash = File::getSeparatorString();
		const auto specialLoc = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);

		return specialLoc.getFullPathName() + slash + "Mrugalla" + slash + "SharedState" + slash + "TheManifestOfWisdom" + slash;
	}

	void ErkenntnisseComp::saveToDisk()
	{
		if (editor.isEmpty())
			return parse("You have to enter some wisdom in order to manifest it.");

		const auto now = Time::getCurrentTime();
		const auto nowStr = now.toString(true, true, false, true).replaceCharacters(" ", "_").replaceCharacters(":", "_");

		File file(getFolder() + nowStr + ".txt");

		if (!file.existsAsFile())
			file.create();
		else
			return parse("Relax! You can only manifest 1 wisdom per minute.");

		file.appendText(editor.getText());
		editor.disable();

		parse("Manifested: " + nowStr);
	}

	void ErkenntnisseComp::parse(String&& msg)
	{
		date.setText(msg);
		date.repaint();
	}

	// JuxtaComp::Jux

	JuxtaComp::Jux::Jux(Utils& u, String&& text, String&& textA, String&& textB) :
		Comp(u),
		labelA(u, text.isEmpty() ? textA : text),
		labelB(u, text.isEmpty() ? textB : "")
	{
		layout.init
		(
			{ 13, 1, 13 },
			{ 1 }
		);

		bool isTitle = text.isNotEmpty();
		labelA.mode = Label::Mode::TextToLabelBounds;
		addAndMakeVisible(labelA);
		if (!isTitle)
		{
			labelA.just = Just::centredRight;
			labelB.just = Just::centredLeft;
			labelB.mode = Label::Mode::TextToLabelBounds;
			addAndMakeVisible(labelB);
		}
		else
			labelA.textCID = ColourID::Interact;
	}

	void JuxtaComp::Jux::paint(Graphics& g)
	{
		const bool isTitle = labelB.getText() == "";
		const auto cID = isTitle ? ColourID::Interact : ColourID::Hover;
		g.setColour(Colours::c(cID));
		const auto thicc = static_cast<int>(utils.thicc);
		const auto h = getHeight();
		const auto w = static_cast<float>(getWidth());
		for (auto y = h - 1; y > h - thicc; --y)
			g.drawHorizontalLine(y, 0.f, w);
	}

	void JuxtaComp::Jux::resized()
	{
		bool isTitle = labelB.getText() == "";

		if (isTitle)
			labelA.setBounds(getLocalBounds());
		else
		{
			layout.resized();

			layout.place(labelA, 0, 0, 1, 1);
			layout.place(labelB, 2, 0, 1, 1);
		}
	}

	// JuxtaComp

	JuxtaComp::JuxtaComp(Utils& u, const ValueTree& vt) :
		Comp(u),
		juxi()
	{
		const auto numJuxi = vt.getNumChildren();
		juxi.reserve(numJuxi);
		for (auto i = 0; i < numJuxi; ++i)
		{
			const auto juxChild = vt.getChild(i);
			juxi.emplace_back(std::make_unique<Jux>
				(
					u,
					juxChild.getProperty("text").toString(),
					juxChild.getProperty("textA").toString(),
					juxChild.getProperty("textB").toString()
					));
		}

		for (auto& j : juxi)
			addAndMakeVisible(*j);
	}

	void JuxtaComp::paint(Graphics&) {}

	void JuxtaComp::resized()
	{
		const auto w = static_cast<float>(getWidth());
		const auto h = static_cast<float>(getHeight());
		const auto x = 0.f;

		const auto numJuxi = juxi.size();
		const auto inc = h / numJuxi;

		auto y = 0.f;
		for (auto& j : juxi)
		{
			j->setBounds(BoundsF(x, y, w, inc).toNearestInt());
			y += inc;
		}
	}

	// ComponentWithBounds

	template<typename CompType>
	ComponentWithBounds::ComponentWithBounds(CompType* _c, BoundsF&& _b, bool _isQuad) :
		c(_c == nullptr ? nullptr : _c),
		b(_b),
		isQuad(_isQuad)
	{}

	// CompModular

	CompModular::CompModular(Utils& u, String&& _tooltip, CursorType ct) :
		Comp(u, std::move(_tooltip), ct)
	{}

	void CompModular::init()
	{
		for (auto& cmp : comps)
			addAndMakeVisible(*cmp.c);
	}

	void CompModular::paint(Graphics&) {}

	void CompModular::resized()
	{
		layout.resized();

		for (auto& cmp : comps)
			if (cmp.c != nullptr)
				layout.place
				(
					*cmp.c,
					cmp.b.getX(),
					cmp.b.getY(),
					cmp.b.getWidth(),
					cmp.b.getHeight(),
					cmp.isQuad
				);
	}

	// NavBar::Node

	NavBar::Node::Node(const ValueTree& _vt, int _x, int _y) :
		vt(_vt),
		x(_x),
		y(_y)
	{}

	// NavBar

	NavBar::Nodes NavBar::makeNodes(const ValueTree& xml, int x, int y)
	{
		Nodes ndes;

		for (auto i = 0; i < xml.getNumChildren(); ++i)
		{
			const auto child = xml.getChild(i);
			if (child.hasType("menu"))
			{
				ndes.push_back({ child, x, y });

				const auto moarNodes = makeNodes(child, x + 1, y + 1);
				for (const auto& n : moarNodes)
					ndes.push_back(n);
				y = ndes.back().y + 1;
			}
		}

		return ndes;
	}

	int NavBar::getDeepestNode() const noexcept
	{
		int d = 0;
		for (const auto& n : nodes)
			if (n.x > d)
				d = n.x;
		return d;
	}

	NavBar::NavBar(Utils& u, const ValueTree& xml) :
		Comp(u, "", CursorType::Default),
		label(u, "Nav"),
		nodes(makeNodes(xml)),
		buttons(),
		numMenus(static_cast<int>(nodes.size())),
		deepestNode(getDeepestNode())
	{
		label.setTooltip("Click on a node in order to navigate to its sub menu.");

		std::vector<int> a, b;
		a.resize(numMenus + 1, 1);
		b.resize(deepestNode + 1, 1);

		layout.init(b, a);

		label.textCID = ColourID::Hover;
		label.mode = Label::Mode::TextToLabelBounds;
		addAndMakeVisible(label);

		buttons.reserve(numMenus);
		for (auto i = 0; i < numMenus; ++i)
		{
			const auto& node = nodes[i].vt;

			buttons.emplace_back(std::make_unique<Button>
			(
				utils, node.getProperty("tooltip").toString()
			));

			auto& btn = *buttons[i];

			makeTextButton(btn, node.getProperty("id").toString(), true, 1);
			auto& lbl = btn.getLabel();
			lbl.just = Just::left;
			lbl.font = getFontDosisMedium();
			lbl.mode = Label::Mode::TextToLabelBounds;
		}

		makeButtonsGroup(buttons, 0);

		for (auto& btn : buttons)
			addAndMakeVisible(*btn);
	}

	void NavBar::init(std::unique_ptr<CompModular>& subMenu, Comp& parent)
	{
		for (auto i = 0; i < numMenus; ++i)
		{
			auto& btn = *buttons[i];

			// make navigation functionality

			btn.onClick.push_back([&sub = subMenu, &prnt = parent, &node = nodes[i]](Button&, const Mouse&)
			{
				auto& utils = prnt.getUtils();

				sub.reset(new CompModular(utils, "", CursorType::Default));

				auto& comps = sub->comps;
				
				{
					const auto& xLayoutProp = node.vt.getProperty("x");
					const auto& yLayoutProp = node.vt.getProperty("y");
					if (xLayoutProp.isUndefined() || yLayoutProp.isUndefined())
						return;

					sub->initLayout(xLayoutProp.toString(), yLayoutProp.toString());
				}

				enum Type { kTitle, kTxt, kColourScheme, kLink, kErkenntnisse, kJuxtaposition, kNumTypes };
				std::array<Identifier, kNumTypes> ids
				{
					"title",
					"txt",
					"colourscheme",
					"link",
					"erkenntnisse",
					"juxtaposition"
				};

				for (auto c = 0; c < node.vt.getNumChildren(); ++c)
				{
					const auto child = node.vt.getChild(c);

					const auto& xProp = child.getProperty("x", 0.f);
					const auto& yProp = child.getProperty("y", 0.f);
					const auto& wProp = child.getProperty("w", 1.f);
					const auto& hProp = child.getProperty("h", 1.f);

					Component* comp{ nullptr };

					if (child.getType() == ids[kTitle])
					{
						auto cmp = new Label(utils, node.vt.getProperty("id").toString());
						cmp->font = getFontLobster();
						cmp->mode = Label::Mode::TextToLabelBounds;

						comp = cmp;
					}
					else if (child.getType() == ids[kTxt])
					{
						auto cmp = new Label(utils, child.getProperty("text").toString());
						cmp->just = getJust(child.getProperty("just").toString());
						cmp->font = getFontDosisRegular();
						cmp->mode = Label::Mode::TextToLabelBounds;
						cmp->setMinFontHeight(12.f);

						comp = cmp;
					}
					else if (child.getType() == ids[kColourScheme])
					{
						auto cmp = new ColourSelector(utils);

						comp = cmp;
					}
					else if (child.getType() == ids[kLink])
					{
						auto cmp = new Button(utils, child.getProperty("tooltip", "").toString());

						makeTextButton(*cmp, child.getProperty("id").toString(), false, true);
						makeURLButton(*cmp, child.getProperty("link"));

						comp = cmp;
					}
					else if (child.getType() == ids[kErkenntnisse])
					{
						auto cmp = new ErkenntnisseComp(utils);

						comp = cmp;
					}
					else if (child.getType() == ids[kJuxtaposition])
					{
						auto cmp = new JuxtaComp(utils, child);

						comp = cmp;
					}

					if (comp != nullptr)
						comps.push_back(ComponentWithBounds(
							{
								comp,
								{
									static_cast<float>(xProp),
									static_cast<float>(yProp),
									static_cast<float>(wProp),
									static_cast<float>(hProp)
								},
								false
							}));
				}

				sub->init();

				prnt.addAndMakeVisible(*sub);
				prnt.getLayout().place(*sub, 1, 2, 2, 1, false);
			});
		}
		// make a temporary mouse event obj lol, hacky af
		Mouse tmp
		(
			*juce::Desktop::getInstance().getMouseSource(0),
			{0.f, 0.f},
			juce::ModifierKeys(),
			1.f, 1.f, 1.f, 1.f, 1.f,
			this, this, juce::Time(),
			{0.f, 0.f}, juce::Time(),
			1, false
		);
		for (auto& oc : buttons.front()->onClick)
			oc(*buttons.front().get(), tmp);
	}

	void NavBar::paint(Graphics&) {}

	void NavBar::resized()
	{
		layout.resized();

		layout.place(label, 0, 0, deepestNode + 1, 1, false);
		
		if (numMenus > 0)
		{
			for (auto i = 0; i < numMenus; ++i)
			{
				auto& btn = *buttons[i];
				const auto& node = nodes[i];

				layout.place
				(
					btn,
					0.f + .5f * node.x,
					1 + node.y,
					1.f + deepestNode - .5f * node.x,
					1,
					false
				);
				btn.getLabel().updateTextBounds();
			}
		}
	}

	// Menu

	Menu::Menu(Utils& u, const ValueTree& xml) :
		CompWidgetable(u, "", CursorType::Default),
		label(u, xml.getProperty("id", "").toString().replaceCharacters("\n"," ")),
		navBar(u, xml),
		subMenu(nullptr)
	{
		setInterceptsMouseClicks(true, true);

		layout.init
		(
			{ 20, 50, 20 },
			{ 20, 50, 750, 20 }
		);

		label.textCID = ColourID::Hover;
		label.mode = Label::Mode::TextToLabelBounds;
		addAndMakeVisible(label);
		addAndMakeVisible(navBar);

		navBar.init(subMenu, *this);

		setOpaque(true);
	}

	void Menu::paint(juce::Graphics& g)
	{
		g.fillAll(Colours::c(ColourID::Bg));
	}

	void Menu::resized()
	{
		layout.resized();
		
		layout.place(label, 1, 1, 1, 1, false);
		layout.place(navBar, 0, 2, 1, 1, false);
		if (subMenu != nullptr)
			layout.place(*subMenu, 1, 2, 2, 1, false);
	}


}