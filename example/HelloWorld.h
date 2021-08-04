/*
* Copyright 2017 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#ifndef HelloWorld_DEFINED
#define HelloWorld_DEFINED

#include "tools/sk_app/Application.h"
#include "tools/sk_app/Window.h"

class SkCanvas;

class HelloWorld : public sk_app::Application, sk_app::Window::Layer {
public:
    HelloWorld(int argc, char** argv, void* platformData);
    ~HelloWorld() override;

    void onIdle() override;

    void onBackendCreated() override;
    void onPaint(SkSurface*) override;
//    bool onKey(skui::Key, skui::InputState, skui::ModifierKey) override;
    bool onChar(SkUnichar c, skui::ModifierKey modifiers) override;
    bool onMouseWheel(float delta, skui::ModifierKey) override;
    bool onMouse(int x, int y, skui::InputState, skui::ModifierKey) override;

private:
    void updateTitle();
    void drawByVertices(SkSurface*);

    sk_app::Window* fWindow;
    sk_app::Window::BackendType fBackendType;

    SkScalar fRotationAngle;
    SkScalar fZoomFactor;
    
    int prevX;
    int prevY;
    int isPan;
    
    float transX;
    float transY;
    
    bool needRefresh;
};

#endif
