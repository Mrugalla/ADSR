#pragma once
#include "TextEditor.h"
#include "../arch/State.h"

#define DebugNumPatches 0

namespace gui
{
	static constexpr int PatchNameWidth = 13;
	static constexpr int PatchAuthorWidth = 8;

	String getFileName(const String& name, const String& author);

	struct Patch :
		public Button
	{
		/*
		utils, name, author
		constructor for making temporary patches
		*/
		Patch(Utils&, const String&, const String&);

		/*
		utils, file
		constructor for reading patch from file */
		Patch(Utils&, const File&);

		/*
		utils, name, author, valueTree
		constructor for saving patch from plugin state */
		Patch(Utils&, const String&, const String&, const ValueTree&);

		bool operator==(const Patch&) const noexcept;

		bool operator!=(const Patch&) const noexcept;

		bool isRemovable() const;

		void resized() override;

		Label name, author;
		File file;

	protected:
		void init();
	};

	static constexpr float PatchRelHeight = 8.f;

	struct Patches :
		public CompScrollable
	{
		using UniquePatch = std::unique_ptr<Patch>;
		using SortFunc = std::function<bool(const UniquePatch&, const UniquePatch&)>;

		Patches(Utils&);

		int getIdx(const Patch&) const noexcept;

		bool contains(const Patch&) const noexcept;

		/* name, author */
		bool save(const String&, const String&);

		bool add(const File&);

		bool removeSelected();

		bool select(Patch*) noexcept;

		int getSelectedIdx() const noexcept;

		const Patch* getSelected() const noexcept;

		Patch& back() noexcept;

		const Patch& back() const noexcept;

		Patch& operator[](int) noexcept;

		const Patch& operator[](int) const noexcept;

		size_t numPatches() const noexcept;

		void sort(const SortFunc&);

		void resized() override;

		void applyFilters(const String&);

		void paint(Graphics&) override;

		void paintList(Graphics&);

	protected:
		std::vector<UniquePatch> patches;
		BoundsF listBounds;
	};

	struct PatchesSortable :
		public Comp
	{
		using UniquePatch = Patches::UniquePatch;
		using SortFunc = Patches::SortFunc;

		PatchesSortable(Utils&);

		int getIdx(const Patch&) const noexcept;

		bool contains(const Patch&) const noexcept;

		/* name, author */
		bool save(const String&, const String&);

		bool add(const File&);

		bool removeSelected();

		bool select(Patch*) noexcept;

		int getSelectedIdx() const noexcept;

		const Patch* getSelected() const noexcept;

		Patch& back() noexcept;

		const Patch& back() const noexcept;

		Patch& operator[](int i) noexcept;

		const Patch& operator[](int i) const noexcept;

		size_t numPatches() const noexcept;

		void sort(const SortFunc&);

		void resized() override;

		void applyFilters(const String&);

		Patches& getPatches() noexcept;

		const Patches& getPatches() const noexcept;

	protected:
		Patches patches;
		Button sortByName, sortByAuthor;

		void paint(Graphics&) override;
	};
	
	struct PatchBrowser :
		public CompScreenshotable
	{
		PatchBrowser(Utils&);

		void setVisible(bool) override;

		void paint(Graphics&) override;

		void resized() override;

		String getSelectedPatchName() const;

	protected:
		Button closeButton;
		Button saveButton, removeButton;

		PatchesSortable patches;

		TextEditor searchBar, authorEditor;

		void savePatch();

		void removePatch();

		void applyFilters();

		void loadPatchesFromDisk(const File&);
	};

	struct ButtonPatchBrowser :
		public Button
	{
		Notify makeNotify(ButtonPatchBrowser&);

		ButtonPatchBrowser(Utils&, PatchBrowser&);

		void paint(Graphics&) override;

	protected:
		PatchBrowser& browser;
	};
}