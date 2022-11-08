#pragma once
#include "ButtonParameterRandomizer.h"

#if PPDHasPatchBrowser
#include "PatchBrowser.h"
#endif

#include "LogoComp.h"
#include "Knob.h"
#include "Menu.h"
#include "MIDICCMonitor.h"
#include "MIDIVoicesComp.h"
#include "LowLevel.h"

#if PPDHasTuningEditor
#include "TuningEditor.h"
#endif

namespace gui
{
	struct HighLevel :
		public Comp
	{
		/* utils, lowlevel, tuningEditor */
		HighLevel(Utils&, LowLevel*
#if PPDHasTuningEditor
			, CompWidgetable*
#endif
		);

		void init();

		void paint(Graphics& g) override;

		void resized() override;
		
	protected:
#if PPDHasPatchBrowser
		PatchBrowser patchBrowser;
		ButtonPatchBrowser patchBrowserButton;
#endif
#if PPDHasTuningEditor
		TuningEditorButton tuningEditorButton;
#endif

		Knob macro;
#if PPDHasClipper
		Button clipper;
#endif
		Button modDepthLocked;
		Button swapParamWithModDepth;
		Button saveModPatch, loadModPatch, removeCurModPatch;

		ButtonParameterRandomizer parameterRandomizer;
#if PPDHasGainIn
		Knob gainIn;
#endif
#if PPDHasGainOut
		Knob gainOut;
#endif
		Knob mix;
#if PPDHasUnityGain && PPDHasGainIn
		Button unityGain;
#endif
		std::vector<std::unique_ptr<Button>> buttonsBottom;

		MIDICCMonitor ccMonitor;
		MIDIVoicesComp midiVoices;

		LowLevel* lowLevel;
		std::unique_ptr<Menu> menu;
		Button menuButton;
		
		std::unique_ptr<FileChooser> fileChooser;

		Notify makeNotify(HighLevel&);
	};
}