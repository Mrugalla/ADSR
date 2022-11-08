#include "PatchBrowser.h"

namespace gui
{
	String getPatchesPath(const PropertiesFile& user)
	{
		const auto& file = user.getFile();
		const auto& pathStr = file.getFullPathName();
		for (auto i = pathStr.length() - 1; i != 0; --i)
			if (pathStr.substring(i, i + 1) == File::getSeparatorString())
				return pathStr.substring(0, i + 1) + "Patches";
		return "";
	}

	File getDirectory(AppProps& props)
	{
		const auto user = props.getUserSettings();
		const auto pathStr = getPatchesPath(*user);
		return { pathStr };
	}

	String getFileName(const String& name, const String& author)
	{
		return author + "_-_" + name + ".patch";
	}

	String getFileName(const Patch& patch)
	{
		return getFileName(patch.name.getText(), patch.author.getText());
	}
	
	// Patch

	Patch::Patch(Utils& u, const String& _name, const String& _author) :
		Button(u, "Click on this patch in order to select it."),
		name(u, _name),
		author(u, _author),
		file()
	{
	}

	Patch::Patch(Utils& u, const File& _file) :
		Button(u, "Click on this patch in order to select it."),
		name(u, ""),
		author(u, ""),
		file(_file)
	{
		const auto fileName = file.getFileNameWithoutExtension();
		// for instance: user_-_best patch ever.patch
		for (auto i = 0; i < fileName.length(); ++i)
		{
			const auto chr = fileName[i];
			if (chr == '_')
				if (fileName.substring(i, i + 3) == "_-_")
				{
					author.setText(fileName.substring(0, i));
					name.setText(fileName.substring(i + 3));
				}
		}
		init();
	}

	Patch::Patch(Utils& u, const String& _name, const String& _author, const ValueTree& vt) :
		Button(u, "Click on this patch in order to select it."),
		name(u, _name),
		author(u, _author),
		file()
	{
		const auto directory = getDirectory(utils.getProps());
		file = File(directory.getFullPathName() + "\\" + getFileName(_name, _author));

		if (file.exists())
			file.deleteFile();
		file.appendText(vt.toXmlString());

		init();
	}

	bool Patch::operator==(const Patch& other) const noexcept
	{
		return other.name.getText() == name.getText()
			&& other.author.getText() == author.getText();
	}

	bool Patch::operator!=(const Patch& other) const noexcept
	{
		return !this->operator==(other);
	}

	bool Patch::isRemovable() const
	{
		return author.getText() != "factory";
	}

	void Patch::resized()
	{
		layout.resized();

		layout.place(name, 1, 0, 1, 1, false);
		layout.place(author, 2, 0, 1, 1, false);
	}

	void Patch::init()
	{
		layout.init
		(
			{ 1, PatchNameWidth, PatchAuthorWidth, 1 },
			{ 1 }
		);

		makeSymbolButton(*this, ButtonSymbol::Empty);

		name.font = getFontDosisMedium();
		author.font = name.font;

		name.textCID = ColourID::Txt;
		author.textCID = ColourID::Hover;

		name.just = Just::centredLeft;
		author.just = Just::centredLeft;

		name.mode = Label::Mode::TextToLabelBounds;
		author.mode = Label::Mode::TextToLabelBounds;

		addAndMakeVisible(name);
		addAndMakeVisible(author);
	}

	// Patches

	Patches::Patches(Utils& u) :
		CompScrollable(u),
		patches(),
		listBounds()
	{
		layout.init
		(
			{ 34, 1 },
			{ 1 }
		);

		if (numPatches() == 0)
		{
			save("Init", "Factory");
		}
	}

	int Patches::getIdx(const Patch& nPatch) const noexcept
	{
		for (auto i = 0; i < numPatches(); ++i)
			if (nPatch == *patches[i])
				return i;
		return -1;
	}

	bool Patches::contains(const Patch& nPatch) const noexcept
	{
		return getIdx(nPatch) != -1;
	}

	bool Patches::save(const String& name, const String& author)
	{
		if (name.isEmpty())
			return false;

		const auto auth = author.isEmpty() ? "user" : author;

		Patch nPatch(utils, name, auth);

		if (contains(nPatch))
			return false;

		patches.push_back(std::make_unique<Patch>(utils, name, auth, utils.savePatch()));
		auto& patch = *patches.back();

		patch.onClick.push_back([&list = patches](Button& btn, const Mouse&)
			{
				for (auto& p : list)
					p->toggleState = 0;

				btn.toggleState = 1;
			});

		patch.onClick.push_back([&, &file = patch.file](Button& btn, const Mouse&)
			{
				auto& utils = btn.utils;
				const auto stream = file.createInputStream();
				if (stream != nullptr)
				{
					const auto vt = ValueTree::fromXml(stream->readEntireStreamAsString());
					utils.loadPatch(vt);
					notify(EvtType::PatchUpdated, nullptr);
				}
			});

		patch.onMouseWheel.push_back([&](Button&, const Mouse& mouse, const MouseWheel& wheel)
			{
				mouseWheelMove(mouse, wheel);
			});

		addAndMakeVisible(patch);
		select(&patch);

		resized();
		repaintWithChildren(this);

		return true;
	}

	bool Patches::add(const File& file)
	{
		Patch nPatch(utils, file);

		if (contains(nPatch))
			return false;

		patches.push_back(std::make_unique<Patch>(utils, file));
		auto& patch = *patches.back();

		patch.onClick.push_back([&list = patches](Button& btn, const Mouse&)
			{
				for (auto& p : list)
					p->toggleState = 0;

				btn.toggleState = 1;
			});

		patch.onClick.push_back([&, &file = patch.file](Button& btn, const Mouse&)
			{
				auto& utils = btn.getUtils();
				const auto stream = file.createInputStream();
				if (stream != nullptr)
				{
					const auto vt = ValueTree::fromXml(stream->readEntireStreamAsString());
					utils.loadPatch(vt);
					notify(EvtType::PatchUpdated, nullptr);
				}
			});

		patch.onMouseWheel.push_back([&](Button&, const Mouse& mouse, const MouseWheel& wheel)
			{
				mouseWheelMove(mouse, wheel);
			});

		addAndMakeVisible(patch);
		select(&patch);

		return true;
	}

	bool Patches::removeSelected()
	{
		const auto directory = getDirectory(utils.getProps());

		auto selected = getSelected();

		if (selected != nullptr && selected->isRemovable())
			for (auto i = 0; i < patches.size(); ++i)
			{
				const auto& patch = *patches[i];

				if (patch == *selected)
				{
					select(nullptr);

					const File file(directory.getFullPathName() + "\\" + getFileName(patch));
					if (file.existsAsFile())
						file.deleteFile();

					patches.erase(patches.begin() + i);

					resized();
					repaintWithChildren(this);

					return true;
				}
			}

		return false;
	}

	bool Patches::select(Patch* nPatch) noexcept
	{
		for (auto& p : patches)
			p->toggleState = 0;

		if (nPatch == nullptr)
			return false;

		for (auto& ptchs : patches)
			if (*ptchs == *nPatch)
			{
				nPatch->toggleState = 1;
				return true;
			}

		return false;
	}

	int Patches::getSelectedIdx() const noexcept
	{
		for (auto i = 0; i < numPatches(); ++i)
			if (patches[i]->toggleState == 1)
				return i;
		return -1;
	}

	const Patch* Patches::getSelected() const noexcept
	{
		auto idx = getSelectedIdx();
		if (idx == -1)
			return nullptr;
		return patches[idx].get();
	}

	Patch& Patches::back() noexcept { return *patches.back(); }

	const Patch& Patches::back() const noexcept { return *patches.back(); }

	Patch& Patches::operator[](int i) noexcept { return *patches[i]; }

	const Patch& Patches::operator[](int i) const noexcept { return *patches[i]; }

	size_t Patches::numPatches() const noexcept { return patches.size(); }

	void Patches::sort(const SortFunc& sortFunc)
	{
		std::sort(patches.begin(), patches.end(), sortFunc);

		resized();
		repaintWithChildren(this);
	}

	void Patches::resized()
	{
		layout.resized();

		layout.place(scrollBar, 1, 0, 1, 1);

		listBounds = layout(0, 0, 1, 1);

		const auto x = listBounds.getX();
		const auto w = listBounds.getWidth();
		const auto h = utils.thicc * PatchRelHeight;
		actualHeight = h * static_cast<float>(numPatches());

		auto y = listBounds.getY() - yScrollOffset;

		for (auto p = 0; p < numPatches(); ++p)
		{
			auto& patch = *patches[p];

			if (patch.isVisible())
			{
				patch.setBounds(BoundsF(x, y, w, h).toNearestInt());
				y += h;
			}
		}
	}

	void Patches::applyFilters(const String& text)
	{
		if (text.isEmpty())
		{
			for (auto& patch : patches)
				patch->setVisible(true);
		}
		else
		{
			const auto txt = text.toLowerCase();

			for (auto& patch : patches)
				patch->setVisible(patch->name.getText().toLowerCase().contains(txt));
		}

		resized();
		repaintWithChildren(this);
	}

	void Patches::paint(Graphics& g)
	{
		if (patches.empty())
		{
			g.setColour(Colours::c(ColourID::Abort));
			g.setFont(getFontLobster().withHeight(24.f));
			g.drawFittedText(
				"sry, this browser does not contain patches yet...",
				getLocalBounds(),
				Just::centred,
				1
			);
			return;
		}

		paintList(g);
	}

	void Patches::paintList(Graphics& g)
	{
		auto x = listBounds.getX();
		auto y = listBounds.getY() - yScrollOffset;
		auto w = listBounds.getWidth();
		auto btm = listBounds.getBottom();
		auto r = utils.thicc * PatchRelHeight;

		g.setColour(Colours::c(ColourID::Txt).withAlpha(.1f));
		for (auto i = 0; i < numPatches(); ++i)
		{
			if (y >= btm)
				return;
			if (i % 2 == 0)
				g.fillRect(x, y, w, r);

			y += r;
		}
	}

	// PatchesSortable

	PatchesSortable::PatchesSortable(Utils& u) :
		Comp(u, "", CursorType::Default),
		patches(u),
		sortByName(u, "Click here to sort all patches by name."),
		sortByAuthor(u, "Click here to sort all patches by author.")
	{
		layout.init(
			{ 21, 1 },
			{ 2, 34 }
		);

		addAndMakeVisible(sortByName);
		addAndMakeVisible(sortByAuthor);

		makeTextButton(sortByName, "NAME");
		makeTextButton(sortByAuthor, "AUTHOR");

		sortByName.onClick.push_back([&](Button& btn, const Mouse&)
			{
				btn.toggleState = btn.toggleState == 0 ? 1 : 0;

				SortFunc sortFunc = [&ts = btn.toggleState](const UniquePatch& a, const UniquePatch& b)
				{
					const auto& pA = a->name.getText();
					const auto& pB = b->name.getText();

					if (ts == 1)
						return pA.compareNatural(pB) > 0;
					else
						return pA.compareNatural(pB) < 0;
				};

				sort(sortFunc);
			});

		sortByAuthor.onClick.push_back([&](Button& btn, const Mouse&)
			{
				btn.toggleState = btn.toggleState == 0 ? 1 : 0;

				SortFunc sortFunc = [&ts = btn.toggleState](const UniquePatch& a, const UniquePatch& b)
				{
					const auto& pA = a->author.getText();
					const auto& pB = b->author.getText();

					if (ts == 1)
						return pA.compareNatural(pB) > 0;
					else
						return pA.compareNatural(pB) < 0;
				};

				sort(sortFunc);
			});

		{
			auto& nLabel = sortByName.getLabel();
			auto& authLabel = sortByAuthor.getLabel();

			nLabel.textCID = ColourID::Txt;
			authLabel.textCID = nLabel.textCID;

			nLabel.font = getFontDosisMedium();
			authLabel.font = nLabel.font;

			nLabel.just = Just::centredLeft;
			authLabel.just = nLabel.just;

			nLabel.mode = Label::Mode::TextToLabelBounds;
			authLabel.mode = nLabel.mode;
		}

		addAndMakeVisible(patches);
	}

	int PatchesSortable::getIdx(const Patch& nPatch) const noexcept
	{
		return patches.getIdx(nPatch);
	}

	bool PatchesSortable::contains(const Patch& nPatch) const noexcept
	{
		return patches.contains(nPatch);
	}

	bool PatchesSortable::save(const String& name, const String& author)
	{
		return patches.save(name, author);
	}

	bool PatchesSortable::add(const File& file)
	{
		return patches.add(file);
	}

	bool PatchesSortable::removeSelected()
	{
		return patches.removeSelected();
	}

	bool PatchesSortable::select(Patch* nPatch) noexcept
	{
		return patches.select(nPatch);
	}

	int PatchesSortable::getSelectedIdx() const noexcept
	{
		return patches.getSelectedIdx();
	}

	const Patch* PatchesSortable::getSelected() const noexcept
	{
		return patches.getSelected();
	}

	Patch& PatchesSortable::back() noexcept { return patches.back(); }

	const Patch& PatchesSortable::back() const noexcept { return patches.back(); }

	Patch& PatchesSortable::operator[](int i) noexcept { return patches[i]; }

	const Patch& PatchesSortable::operator[](int i) const noexcept { return patches[i]; }

	size_t PatchesSortable::numPatches() const noexcept { return patches.numPatches(); }

	void PatchesSortable::sort(const SortFunc& sortFunc)
	{
		patches.sort(sortFunc);
	}

	void PatchesSortable::resized()
	{
		layout.resized();

		auto buttonArea = layout(0, 0, 1, 1);
		{
			auto x = buttonArea.getX();
			auto y = buttonArea.getY();
			auto w = buttonArea.getWidth();
			auto h = buttonArea.getHeight();

			const auto bothWidth = 1.f / static_cast<float>(PatchNameWidth + PatchAuthorWidth);
			const auto nameW = w * static_cast<float>(PatchNameWidth) * bothWidth;
			const auto authorW = w * static_cast<float>(PatchAuthorWidth) * bothWidth;

			sortByName.setBounds(BoundsF(x, y, nameW, h).toNearestInt());
			x += nameW;
			sortByAuthor.setBounds(BoundsF(x, y, authorW, h).toNearestInt());
		}

		layout.place(patches, 0, 1, 2, 1, false);
	}

	void PatchesSortable::applyFilters(const String& text)
	{
		patches.applyFilters(text);
	}

	Patches& PatchesSortable::getPatches() noexcept { return patches; }

	const Patches& PatchesSortable::getPatches() const noexcept { return patches; }

	void PatchesSortable::paint(Graphics&) {}

	// PatchBrowser
	
	PatchBrowser::PatchBrowser(Utils& u) :
		CompScreenshotable(u),

		closeButton(u, "Click here to close the browser."),
		saveButton(u, "Click here to save this patch."),
		removeButton(u, "Click here to remove this patch."),

		patches(u),

		searchBar(u, "Define a name or search for a patch!", "Init.."),
		authorEditor(u, "Define your author name if you want to save a patch!", "Author..")
	{
		setInterceptsMouseClicks(true, true);

		{
			auto& props = utils.getProps();
			const auto& user = *props.getUserSettings();
			const auto lastAuthorName = user.getValue("patchBrowserLastAuthor", "user");
			authorEditor.setText(lastAuthorName);
			const auto pathStr = getPatchesPath(user);
			const File directory(pathStr);
			directory.createDirectory();
			loadPatchesFromDisk(directory);
		}

		layout.init
		(
			{ 1, 3, 34, 13, 3, 3, 1 },
			{ 1, 3, 34, 21, 1 }
		);

		makeTextButton(closeButton, "X", false);
		closeButton.getLabel().mode = Label::Mode::TextToLabelBounds;
		closeButton.getLabel().textCID = ColourID::Abort;
		closeButton.onClick.push_back([&](Button&, const Mouse&)
			{
				setVisible(false);
			});

		makeTextButton(saveButton, "save", false);
		saveButton.getLabel().mode = Label::Mode::TextToLabelBounds;
		saveButton.onClick.push_back([&](Button&, const Mouse&)
			{
				savePatch();
			});

		searchBar.onReturn = [&]()
		{
			savePatch();
			return true;
		};

		searchBar.onType = [&]()
		{
			applyFilters();
			return true;
		};

		authorEditor.onReturn = searchBar.onReturn;

		makeTextButton(removeButton, "rmv", false);
		removeButton.getLabel().textCID = ColourID::Abort;
		removeButton.onClick.push_back([&](Button&, const Mouse&)
			{
				removePatch();
			});

		addAndMakeVisible(closeButton);
		addAndMakeVisible(saveButton);
		addAndMakeVisible(removeButton);
		addAndMakeVisible(searchBar);
		addAndMakeVisible(authorEditor);
		addAndMakeVisible(patches);

		onScreenshotFX.push_back([](Graphics& g, Image& img)
			{
				imgPP::blur(img, g, 7);

				auto bgCol = Colours::c(ColourID::Bg);

				if (bgCol.getPerceivedBrightness() < .5f)
					for (auto y = 0; y < img.getHeight(); ++y)
						for (auto x = 0; x < img.getWidth(); ++x)
							img.setPixelAt
							(x, y,
								img.getPixelAt(x, y)
								.withMultipliedSaturation(.4f)
								.withMultipliedBrightness(.4f)
							);
				else
					for (auto y = 0; y < img.getHeight(); ++y)
						for (auto x = 0; x < img.getWidth(); ++x)
							img.setPixelAt
							(x, y,
								img.getPixelAt(x, y)
								.withMultipliedSaturation(.4f)
								.withMultipliedBrightness(1.5f)
							);
			});

		patches.select(nullptr);

#if DebugNumPatches != 0
		Random rand;
		for (auto i = 0; i < DebugNumPatches; ++i)
		{
			String strA, strB;
			appendRandomString(strA, rand, 12);
			searchBar.setText(strA);
			appendRandomString(strB, rand, 12);
			authorEditor.setText(strB);
			savePatch();
		}
#endif
	}

	void PatchBrowser::setVisible(bool e)
	{
		if (e)
		{
			notify(EvtType::BrowserOpened);
			takeScreenshot();
			Comp::setVisible(e);
			authorEditor.enable();
			searchBar.enable();
		}
		else
		{
			notify(EvtType::BrowserClosed);
			searchBar.disable();
			authorEditor.disable();
			Comp::setVisible(e);
		}
	}

	void PatchBrowser::paint(Graphics& g)
	{
		CompScreenshotable::paint(g);
	}

	void PatchBrowser::resized()
	{
		CompScreenshotable::resized();

		layout.resized();

		layout.place(closeButton, 1, 1, 1, 1, true);
		layout.place(saveButton, 4, 1, 1, 1, true);
		layout.place(removeButton, 5, 1, 1, 1, true);

		layout.place(searchBar, 2, 1, 1, 1, false);
		layout.place(authorEditor, 3, 1, 1, 1, false);

		layout.place(patches, 1, 2, 5, 2, false);

	}

	String PatchBrowser::getSelectedPatchName() const
	{
		const auto patch = patches.getSelected();
		if (patch != nullptr)
			return patch->name.getText();
		return "init";
	}

	void PatchBrowser::savePatch()
	{
		const auto& name = searchBar.getText();
		const auto& author = authorEditor.getText();
		if (patches.save(name, author))
		{
			auto& props = utils.getProps();
			auto& user = *props.getUserSettings();
			user.setValue("patchBrowserLastAuthor", author);
		}

		searchBar.clear();
		applyFilters();
	}

	void PatchBrowser::removePatch()
	{
		patches.removeSelected();
	}

	void PatchBrowser::applyFilters()
	{
		const auto& str = searchBar.getText();

		patches.applyFilters(str);
	}

	void PatchBrowser::loadPatchesFromDisk(const File& directory)
	{
		const auto fileTypes = File::TypesOfFileToFind::findFiles;
		const String extension(".patch");
		const auto wildCard = "*" + extension;
		const RangedDirectoryIterator files
		(
			directory,
			true,
			wildCard,
			fileTypes
		);

		for (const auto& it : files)
			patches.add(it.getFile());
	}

	// ButtonPatchBrowser

	Notify ButtonPatchBrowser::makeNotify(ButtonPatchBrowser& _bpb)
	{
		return [&bpb = _bpb](EvtType evt, const void*)
		{
			if (evt == EvtType::PatchUpdated)
			{
				bpb.getLabel().setText(bpb.browser.getSelectedPatchName());
				bpb.repaint();
			}
		};
	}

	ButtonPatchBrowser::ButtonPatchBrowser(Utils& u, PatchBrowser& _browser) :
		Button(u, "Click here to open the patch browser.", makeNotify(*this)),
		browser(_browser)
	{
		makeTextButton(*this, browser.getSelectedPatchName(), false);
		onClick.push_back([&](Button&, const Mouse&)
			{
				const auto e = browser.isVisible();
				if (e)
					browser.setVisible(false);
				else
				{
					browser.setVisible(true);
				}
			});
	}

	void ButtonPatchBrowser::paint(Graphics& g)
	{
		Button::paint(g);
		g.setColour(Colours::c(ColourID::Hover));
		const auto thicc = utils.thicc;
		const auto bounds = getLocalBounds().toFloat().reduced(thicc);
		g.drawRoundedRectangle(bounds, thicc, thicc);
	}
}

#undef DebugNumPatches