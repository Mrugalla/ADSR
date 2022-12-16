#pragma once

#include "gui/Shader.h"
#include "gui/ContextMenu.h"
#include "gui/HighLevel.h"
#include "gui/Tooltip.h"
#include "gui/ToastComponent.h"

#if PPDHasTuningEditor
#include "gui/TuningEditor.h"
#endif

namespace gui
{
    struct Editor :
        public juce::AudioProcessorEditor
    {
        static constexpr int MinWidth = 100, MinHeight = 100;

        Editor(audio::Processor&);
        
        ~Editor();
        
        void paint(Graphics& g) override;
        
        void resized() override;

        void mouseEnter(const Mouse&) override;
        void mouseExit(const Mouse&) override;
        void mouseDown(const Mouse&) override;
        void mouseDrag(const Mouse&) override;
        void mouseUp(const Mouse&) override;
        void mouseWheelMove(const Mouse&, const MouseWheel&) override;

        audio::Processor& audioProcessor;
    
protected:
        Layout layout;
        Utils utils;

        Image bgImage;
        Evt notify;
        Button imgRefresh;

        Tooltip tooltip;

        Label pluginTitle;

        LowLevel lowLevel;
#if PPDHasTuningEditor
        TuningEditor tuningEditor;
#endif
        HighLevel highLevel;

        ContextMenuKnobs contextMenuKnobs;
        ContextMenuButtons contextMenuButtons;

        TextEditorKnobs editorKnobs;

        ToastComp toast;

        bool bypassed;
        Shader shadr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
        //JUCE_LEAK_DETECTOR(Editor)
        //JUCE_HEAVYWEIGHT_LEAK_DETECTOR(Editor)
    private:
        void saveBounds();

        Notify makeNotify(Editor&);

        /* forced */
        void updateBgImage(bool);
    };
}